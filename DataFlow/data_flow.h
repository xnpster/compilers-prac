#pragma once
#include<vector>
#include<set>
#include<memory>

#include "../qbelib.h"

template <class DataType>
class DataFlowAnalysisNode {
    static const int CNT_NOTINIT = 0;

    int in_cnt = CNT_NOTINIT, out_cnt = CNT_NOTINIT;

    std::set<int> rollback;
    std::set<int> ignore_rollback;

    std::set<std::shared_ptr<DataFlowAnalysisNode<DataType>>> prev_blocks;

    Blk* bb;
public:
    DataFlowAnalysisNode();

    void doStep();

    virtual DataType getIn() = 0;
    virtual DataType getOut() = 0;

    virtual bool addIn(const DataType& data) = 0;
    virtual bool addOut(const DataType& data) = 0;

    virtual bool forwardData(const DataType& data) = 0;

    bool newerThan(const std::shared_ptr<DataFlowAnalysisNode<DataType>> block) const { return out_cnt > block->in_cnt; }

    int getInCnt() { return in_cnt; }
    int getOutCnt() { return out_cnt; }

    void setInCnt(int cnt) { in_cnt = cnt; }
    void setOutCnt(int cnt) { out_cnt = cnt; }
    static int getStartCounter() { return CNT_NOTINIT; }

    std::set<int>& getRollback() { return rollback; }
    std::set<int>& getIgnoreRollback() { return ignore_rollback; }

    std::set<std::shared_ptr<DataFlowAnalysisNode<DataType>>>& getPrevBlocks() { return prev_blocks; }

    Blk* getBlock() { return bb; }
    void setBlock(Blk* b) { bb = b; }
};

template <class NodeType>
class DataFlowAnalysis {
protected:
    std::vector<std::shared_ptr<NodeType>> nodes;

    virtual std::shared_ptr<NodeType> createNode(Blk* block) = 0;
public:
    virtual void fit(const std::vector<Blk*>& blocks) = 0;
    void analyze();

    const std::vector<std::shared_ptr<NodeType>>& getNodes() { return nodes; }

    ~DataFlowAnalysis();
};

#include "data_flow_impl.h"