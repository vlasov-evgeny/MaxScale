/*
 * Copyright (c) 2019 MariaDB Corporation Ab
 *
 * Use of this software is governed by the Business Source License included
 * in the LICENSE.TXT file and at www.mariadb.com/bsl11.
 *
 * Change Date: 2023-01-01
 *
 * On the date above, in accordance with the Business Source License, use
 * of this software will be governed by version 2 or later of the General
 * Public License.
 */
#pragma once

#include "smartrouter.hh"

#include <maxsql/packet_tracker.hh>
#include <maxscale/queryclassifier.hh>
#include <maxbase/host.hh>

#include <iostream>

class SmartRouter;

/** Currently SmartRouter is configured like this (star means many):
 *  SmartRouter -*> ServerAsService -> MaxscaleRouter -*> Server
 *  For the time being the limitation is that the tail router must be RWSplit.
 *  This will change once we implement it so that SmartRouter can call the tail router directly.
 *  Although the assumption is one RowServer and one ColumnServer, the code does not assume that,
 *  it simply forces you to state which Cluster is the master.
 *  Currently SmartRouter fails if any Cluster fails. That need not be the case, Clusters could
 *  be marked "is_critical" meaning non-crititcal non-masters, could be allowed to fail.
 */

class SmartRouterSession : public mxs::RouterSession, private mxs::QueryClassifier::Handler
{
public:
    static SmartRouterSession* create(SmartRouter* pRouter, MXS_SESSION* pSession);

    virtual ~SmartRouterSession();
    SmartRouterSession(const SmartRouterSession&) = delete;
    SmartRouterSession& operator=(const SmartRouterSession&) = delete;

    int  routeQuery(GWBUF* pBuf);
    void close();
    void clientReply(GWBUF* pPacket, DCB* pDcb);
    void handleError(GWBUF* pPacket,
                     DCB* pProblem,
                     mxs_error_action_t action,
                     bool* pSuccess);

private:
    enum class Mode {Idle, Query, MeasureQuery, CollectResults};

    /** struct Cluster represents a cluster of mariadb servers as a Maxscale internal Server.
     *  TODO In the next iteration a directly callable "Thing" should be implemented (Router, Backend
     *       Server - the terms are overused and confusing, maybe a new thing called MariaDB).
     */
    struct Cluster
    {
        Cluster(SERVER_REF* b, DCB* pDcb, bool is_master)
            : host {b->server->address, b->server->port}
            , pBackend(b)
            , pDcb(pDcb)
            , is_master(is_master)
        {
        }

        Cluster(const Cluster&) = delete;
        Cluster& operator=(const Cluster&) = delete;

        Cluster(Cluster&&) = default;
        Cluster& operator=(Cluster&&) = default;

        maxbase::Host host;
        SERVER_REF*   pBackend;
        DCB*          pDcb;
        bool          is_master;
        bool          is_replying_to_client;

        maxsql::PacketTracker tracker;
    };

    using Clusters = std::vector<Cluster>;

    SmartRouterSession(SmartRouter*, MXS_SESSION* pSession, Clusters clusters);

    std::vector<maxbase::Host> hosts() const;

    // The write functions initialize Cluster flags and Cluster::ProtocolTracker.
    bool write_to_host(const maxbase::Host& host, GWBUF* pBuf);
    bool write_to_master(GWBUF* pBuf);
    bool write_to_all(GWBUF* pBuf, Mode mode);
    bool write_split_packets(GWBUF* pBuf);

    void kill_all_others(const Cluster& cluster);

    bool expecting_request_packets() const;
    bool expecting_response_packets() const;
    bool all_clusters_are_idle() const;     // no clusters expect packets

    // QueryClassifier::Handler overrides, not used.
    bool lock_to_master() override;
    bool is_locked_to_master() const override;
    bool supports_hint(HINT_TYPE hint_type) const override;

    SmartRouter& m_router;

    Mode   m_mode = Mode::Idle;
    GWBUF* m_pDelayed_packet = nullptr;

    DCB*                 m_pClient_dcb;
    Clusters             m_clusters;
    mxs::QueryClassifier m_qc;
    struct Measurement
    {
        maxbase::TimePoint start;
        std::string        canonical;
    };
    Measurement m_measurement;
};
