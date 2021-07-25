/*
 * Copyright (c) 2011-2013 ARM Limited
 * All rights reserved
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
 * Copyright (c) 2006 The Regents of The University of Michigan
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Authors: Ali Saidi
 *          Steve Reinhardt
 *          Andreas Hansson
 */

/**
 * @file
 * Declaration of a memory-mapped bridge that connects a master
 * and a slave through a request and response queue.
 */

#ifndef SHIFTLAB_PREDICTOR_BACKEND_H__
#define SHIFTLAB_PREDICTOR_BACKEND_H__

#include <deque>

#include "base/types.hh"
#include "mem/predictor/CacheLine.hh"
#include "mem/predictor/Constants.hh"
#include "mem/predictor/Declarations.hh"
#include "mem/predictor/CompletedWriteEntry.hh"
#include "mem/port.hh"
#include "params/PredictorBackend.hh"
#include "debug/PredictorBackend.hh"
#include "debug/PredictorBackendLogic.hh"
#include "sim/clocked_object.hh"

#include <fstream>


/**
 * A bridge is used to interface two different crossbars (or in general a
 * memory-mapped master and slave), with buffering for requests and
 * responses. The bridge has a fixed delay for packets passing through
 * it and responds to a fixed set of address ranges.
 *
 * The bridge comprises a slave port and a master port, that buffer
 * outgoing responses and requests respectively. Buffer space is
 * reserved when a request arrives, also reserving response space
 * before forwarding the request. If there is no space present, then
 * the bridge will delay accepting the packet until space becomes
 * available.
 */
class PredictorBackend : public ClockedObject
{
  public:
    /* Written in a hurry, sorry 😢*/
    using CompletedWrites_Q = std::unordered_map<Addr_t, std::deque<CompletedWriteEntry>>;
    using CTKey_t = hash_t;
    using CTValue_t = uint16_t;
    using ConfTable_t = std::unordered_map<CTKey_t, CTValue_t>;
    std::ofstream myFile;
    static ConfTable_t confidenceTable;
    static std::ofstream resultBufferSize;
  protected:
    static uint64_t capacityEvictionStatic;
    std::string enableNonVolatileDump = "0";

    static int MAX_COMPLETED_QUEUE_LINE_SIZE;
    /**
     * A deferred packet stores a packet along with its scheduled
     * transmission time
     */
    class DeferredPacket
    {

      public:

        const Tick tick;
        const PacketPtr pkt;

        DeferredPacket(PacketPtr _pkt, Tick _tick) : tick(_tick), pkt(_pkt)
        { }
    };

    // Forward declaration to allow the slave port to have a pointer
    class PBMasterPort;

    /**
     * The port on the side that receives requests and sends
     * responses. The slave port has a set of address ranges that it
     * is responsible for. The slave port also has a buffer for the
     * responses not yet sent.
     */
    class PBSlavePort : public SlavePort
    {

      private:

        /** The bridge to which this port belongs. */
        PredictorBackend& pb;

        /**
         * Master port on the other side of the bridge.
         */
        PBMasterPort& masterPort;

        /** Minimum request delay though this bridge. */
        const Cycles delay;

        /** Address ranges to pass through the bridge */
        const AddrRangeList ranges;

        /**
         * Response packet queue. Response packets are held in this
         * queue for a specified delay to model the processing delay
         * of the bridge. We use a deque as we need to iterate over
         * the items for functional accesses.
         */
        std::deque<DeferredPacket> transmitList;

        /** Counter to track the outstanding responses. */
        unsigned int outstandingResponses;

        /** If we should send a retry when space becomes available. */
        bool retryReq;

        /** Max queue size for reserved responses. */
        unsigned int respQueueLimit;

        /**
         * Upstream caches need this packet until true is returned, so
         * hold it for deletion until a subsequent call
         */
        std::unique_ptr<Packet> pendingDelete;

        /**
         * Is this side blocked from accepting new response packets.
         *
         * @return true if the reserved space has reached the set limit
         */
        bool respQueueFull() const;

        /**
         * Handle send event, scheduled when the packet at the head of
         * the response queue is ready to transmit (for timing
         * accesses only).
         */
        void trySendTiming();

        /** Send event for the response queue. */
        EventFunctionWrapper sendEvent;

      public:

        /**
         * Constructor for the PBSlavePort.
         *
         * @param _name the port name including the owner
         * @param _bridge the structural owner
         * @param _masterPort the master port on the other side of the bridge
         * @param _delay the delay in cycles from receiving to sending
         * @param _resp_limit the size of the response queue
         * @param _ranges a number of address ranges to forward
         */
        PBSlavePort(const std::string& _name, PredictorBackend& _pb,
                        PBMasterPort& _masterPort, Cycles _delay,
                        int _resp_limit, std::vector<AddrRange> _ranges);

        /**
         * Queue a response packet to be sent out later and also schedule
         * a send if necessary.
         *
         * @param pkt a response to send out after a delay
         * @param when tick when response packet should be sent
         */
        void schedTimingResp(PacketPtr pkt, Tick when);

        /**
         * Retry any stalled request that we have failed to accept at
         * an earlier point in time. This call will do nothing if no
         * request is waiting.
         */
        void retryStalledReq();

      protected:

        /** When receiving a timing request from the peer port,
            pass it to the bridge. */
        bool recvTimingReq(PacketPtr pkt);

        /** When receiving a retry request from the peer port,
            pass it to the bridge. */
        void recvRespRetry();

        /** When receiving a Atomic requestfrom the peer port,
            pass it to the bridge. */
        Tick recvAtomic(PacketPtr pkt);

        /** When receiving a Functional request from the peer port,
            pass it to the bridge. */
        void recvFunctional(PacketPtr pkt);

        /** When receiving a address range request the peer port,
            pass it to the bridge. */
        AddrRangeList getAddrRanges() const;
    };


    /**
     * Port on the side that forwards requests and receives
     * responses. The master port has a buffer for the requests not
     * yet sent.
     */
    class PBMasterPort : public MasterPort
    {

      private:

        /** The bridge to which this port belongs. */
        PredictorBackend& pb;

        /**
         * The slave port on the other side of the bridge.
         */
        PBSlavePort& slavePort;

        /** Minimum delay though this bridge. */
        const Cycles delay;

        /**
         * Request packet queue. Request packets are held in this
         * queue for a specified delay to model the processing delay
         * of the bridge.  We use a deque as we need to iterate over
         * the items for functional accesses.
         */
        std::deque<DeferredPacket> transmitList;

        /** Max queue size for request packets */
        const unsigned int reqQueueLimit;

        /**
         * Handle send event, scheduled when the packet at the head of
         * the outbound queue is ready to transmit (for timing
         * accesses only).
         */
        void trySendTiming();

        /** Send event for the request queue. */
        EventFunctionWrapper sendEvent;

      public:

        /**
         * Constructor for the PBMasterPort.
         *
         * @param _name the port name including the owner
         * @param _bridge the structural owner
         * @param _slavePort the slave port on the other side of the bridge
         * @param _delay the delay in cycles from receiving to sending
         * @param _req_limit the size of the request queue
         */
        PBMasterPort(const std::string& _name, PredictorBackend& _pb,
                         PBSlavePort& _slavePort, Cycles _delay,
                         int _req_limit);

        /**
         * Is this side blocked from accepting new request packets.
         *
         * @return true if the occupied space has reached the set limit
         */
        bool reqQueueFull() const;

        /**
         * Queue a request packet to be sent out later and also schedule
         * a send if necessary.
         *
         * @param pkt a request to send out after a delay
         * @param when tick when response packet should be sent
         */
        void schedTimingReq(PacketPtr pkt, Tick when);

        /**
         * Check a functional request against the packets in our
         * request queue.
         *
         * @param pkt packet to check against
         *
         * @return true if we find a match
         */
        bool trySatisfyFunctional(PacketPtr pkt);

      protected:

        /** When receiving a timing request from the peer port,
            pass it to the bridge. */
        bool recvTimingResp(PacketPtr pkt);

        /** When receiving a retry request from the peer port,
            pass it to the bridge. */
        void recvReqRetry();
    };

    /** Slave port of the bridge. */
    PBSlavePort slavePort;

    /** Master port of the bridge. */
    PBMasterPort masterPort;
    
    Stats::Scalar totalPWrites;
    Stats::Scalar correctlyPredictedChunks;
    Stats::Scalar uniqPCSigCount;
    Stats::Scalar correctlyPredictedPWrites;
    Stats::Scalar correctlyPredictedFreeWrites;
    Stats::Scalar correctlyPredNonFreeWr;
    Stats::Scalar dataMissPredictedPWrites;
    Stats::Scalar addrMissPredictedPWrites;
    Stats::Scalar incorrectlyPredictedPWrites;
    Stats::Scalar nonPredictedPWrites;
    Stats::Scalar addrPredictions;
    Stats::Scalar invalidatedPredictions;
    Stats::Average avgDataMatchForAddrMatch;
    Stats::Distribution pmWriteMatchDistance;
    Stats::Distribution addrPmWriteMatchDistance;
    Stats::Scalar correctConst0Pred;
    Stats::Scalar resultBufferCapacityEvictions;
    Stats::Distribution addrMatchDist;
    Stats::Distribution dataMatchDist;
    Stats::Distribution ihbPatternMatchIndex;
    Stats::Scalar validChunks;
    Stats::Distribution writebackDistStat;
    Stats::Distribution writebackDistStatMicro;

    std::ofstream hashStats;

    static size_t RESULT_BUFFER_MAX_SIZE;

  public:
    static bool usePredictor;
    static CompletedWrites_Q completedWrites;
    static std::unordered_map<Addr_t, Tick> completedWritesManager;

    Port &getPort(const std::string &if_name,
                  PortID idx=InvalidPortID) override;

    void init() override;

    typedef PredictorBackendParams Params;

    PredictorBackend(Params *p);
    ~PredictorBackend();
    
    static Addr_t getCompWriteKey(Addr_t addr);

    void predictorHandleRequest(PacketPtr pkt);
    void handleNonVolatileWrite(PacketPtr pkt);
    void dumpTrace(PacketPtr pkt);

    /* For finding distance between a write and a writeback request */
    std::unordered_map<Addr_t, Tick> writebackDistMap;

    /**
     * Broadcasts information about a prediction to all the predictor
     * frontends using SharedArea.
    */
    static void broadcastPrediction(hash_t hash, bool addrPredicted, bool dataPredicted);

    static void addCompletedWrite(CompletedWriteEntry entry);

    /**
     * Caclulates the confidence for an NVM address based on the previous writes.
     * Works with both Virtual and Physical address
    */
    static size_t getConfidenceForLoc(hash_t hash);

    static bool isPktEqualCompletedEntryAddr(PacketPtr pkt, CompletedWriteEntry completedEntry);
    static bool isPktEqualCompletedEntryData(PacketPtr pkt, CompletedWriteEntry completedEntry);
    static bool isPktEqualCompletedEntry(PacketPtr pkt, CompletedWriteEntry completedEntry);
    static void updatePCConf(PacketPtr pkt, CompletedWriteEntry completedEntry);
    size_t getMatchingChunkCount(PacketPtr pkt, CompletedWriteEntry completedEntry);
    static void initConf(hash_t hash);
    static bool predictorEnabled;
    void update_stats_for_const_pred(CompletedWriteEntry completedEntry);
    static Addr getCompletedAddrToEvict();
    void invalidateAllAddr();
    static std::unordered_map<PC_t, int> addrMatches;
    std::bitset<DATA_CHUNK_COUNT> dataChunkMatchVec(CompletedWriteEntry completedEntry, PacketPtr ptr);
    std::bitset<DATA_CHUNK_COUNT> dataChunkConstVec(CompletedWriteEntry completedEntry);
    
    void updateConstChunks(hash_t maxDataMatchHash, Addr_t addr, PacketPtr pkt);
};

#endif // SHIFTLAB_PREDICTOR_BACKEND_H__
