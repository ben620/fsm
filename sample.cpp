#include "Fsm.h"

class NavStateBase : public tfsm::StateBase
{
};


struct EvtPosChange : public tfsm::Event
{
};

class NavFsm : public tfsm::FsmWithNodes<StPrepare, StFindPath, StNaving, StInterNav, StUploading>, public FsmInterFace
{
public:
    NavFsm() = default;

    template <class EVT>
    void Dispatch(EVT&& e)
    {
        tfsm::FsmWithNodes::Dispatch<NavStateBase, EVT>(std::forward<EVT>(e));
    }
};

