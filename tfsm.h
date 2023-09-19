#pragma once
/**
* based on idea from https://github.com/digint/tinyfsm
* no global static member is used
*/
#include <tuple>


#ifndef FORCEINLINE
#   ifdef _MSC_VER
#       define FORCEINLINE __forceinline
#   elif defined __GNUC__
#       define FORCEINLINE __inline__ __attribute__((always_inline))
#   else
#       define FORCEINLINE inline
#   endif
#endif

namespace tfsm {

class FSMBase
{
public:
    virtual ~FSMBase() = default;
};

class Event{};

class StateBase
{
public:
    virtual void OnEnter() {}
    virtual void OnLeave() {}
    virtual void Reset() {}
    void React(const Event& e) {}
    void React(Event&& e) {}

protected:
    void SetFsmPtr(FSMBase* fsm)
    {
        _fsm = fsm;
    }

    FSMBase* _fsm = nullptr;

private:
    template <class ...STATES_T>
    friend class FsmWithNodes;
};

//idea from https://devblogs.microsoft.com/oldnewthing/20200629-00/?p=103910
namespace tuple_type_index_impl
{
    template<typename T, typename Tuple>
    struct GetTupleTypeIndexImp;

    template<typename T>
    struct GetTupleTypeIndexImp<T, std::tuple<>>
    {
        static constexpr std::size_t value = 0;
    };

    template<typename T, typename... Rest>
    struct GetTupleTypeIndexImp<T, std::tuple<T, Rest...>>
    {
        static constexpr std::size_t value = 0;
        using RestTuple = std::tuple<Rest...>;
        static_assert(
            GetTupleTypeIndexImp<T, RestTuple>::value ==
            std::tuple_size_v<RestTuple>,
            "type appears more than once in tuple");
    };

    template<typename T, typename First, typename... Rest>
    struct GetTupleTypeIndexImp<T, std::tuple<First, Rest...>>
    {
        using RestTuple = std::tuple<Rest...>;
        static constexpr std::size_t value = 1 +
            GetTupleTypeIndexImp<T, RestTuple>::value;
    };
}

template<typename T, typename Tuple>
struct TupleTypeIndex
{
    static constexpr std::size_t value =
        tuple_type_index_impl::GetTupleTypeIndexImp<T, Tuple>::value;
    static_assert(value < std::tuple_size_v<Tuple>,
        "type does not appear in tuple");
};

template<typename T, typename Tuple>
inline constexpr std::size_t TupleTypeIndexV = TupleTypeIndex<T, Tuple>::value;

template <typename T, typename Tuple>
struct TupleHasType;

template <typename T, typename... Us>
struct TupleHasType<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};

template <class ...STATES_T>
class FsmWithNodes : public FSMBase
{
public:
    virtual ~FsmWithNodes() = default;
    FsmWithNodes(const FsmWithNodes& r) = delete;
    FsmWithNodes& operator=(const FsmWithNodes& r) = delete;

    template <typename S>
    FORCEINLINE void Transit()
    {
        auto* s = &std::get<TupleTypeIndexV<S, StatesTuple_T>>(_states);
        if (s == _curState)
        {
            return;
        }

        _curState->OnLeave();
        _curState = s;
        s->OnEnter();
    }

    template <class StateNodeT, class EVT>
    FORCEINLINE void Dispatch(EVT&& e)
    {
        if (_curState == nullptr)
        {
            return;
        }

        static_cast<StateNodeT*>(_curState)->React(std::forward<EVT>(e));
    }

    void Reset()
    {
        ResetImp<sizeof ...(STATES_T) - 1>();
    }

protected:
    using StatesTuple_T = std::tuple<STATES_T...>;

    template <typename S> requires TupleHasType<S, StatesTuple_T>::value
    void SetInitState()
    {
        _curState = &std::get<TupleTypeIndexV<S, StatesTuple_T>>(_states);
        _curState->OnEnter();
    }

    FsmWithNodes()
    {
        InitSates<sizeof...(STATES_T) - 1>();
    }

private:
    template<int I>
    void InitSates()
    {
        std::get<I>(_states).SetFsmPtr(this);
        if constexpr (I > 0)
        {
            InitSates<I - 1>();
        }
    }

    template<int I>
    void ResetImp()
    {
        if constexpr (I >= 0)
        {
            std::get<I>(_states).Reset();
        }
        if constexpr (I > 0)
        {
            ResetImp<I - 1>();
        }
    }

private:
    StateBase* _curState = nullptr;
    StatesTuple_T _states;
};

}


