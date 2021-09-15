#include <tuple>
#include <iostream>
#include <optional>
#include <functional>
#include <cassert>

template <typename... Args>
struct PipelineConsumer
{
    virtual void consume(Args...) = 0;
};

template <typename... Args>
struct PipelineProducer
{
    void produce(Args... args)
    {
        assert(consumer_);
        if (!consumer_)
            return;

        consumer_->get().consume(args...);
    }

    void setConsumer(PipelineConsumer<Args...>& consumer)
    {
        consumer_ = consumer;
    }

    std::optional<std::reference_wrapper<PipelineConsumer<Args...>>> consumer_;
};

template <typename... Args>
constexpr auto makeProducerType(std::tuple<Args...>)
{
    return PipelineProducer<Args...>();
}

template <typename Producer, typename F, typename... Args>
constexpr auto makeComponentType(std::tuple<Args...>, F&& f)
{
    struct Component : PipelineConsumer<Args...>, Producer
    {
        Component(const F& f) : f(f)
        {
        }

        void consume(Args... args) override
        {
            auto t = f(args...);
            std::apply([this](auto&&... args){Producer::produce(args...);}, t);
        }

        F f;
    };

    return Component(f);
}

template <typename F, typename... Args>
constexpr auto makeConsumerType(std::tuple<Args...>, F&& f)
{
    struct Component : PipelineConsumer<Args...>
    {
        Component(const F& f) : f(f)
        {
        }

        void consume(Args... args) override
        {
            f(args...);
        }

        F f;
    };

    return Component(f);
}

template <typename T, typename R, typename... Args>
constexpr std::tuple<Args...> extractFunctionArgs(R (T::*)(Args...) const)
{
    return std::tuple<Args...>();    
}

template <typename T, typename R, typename... Args>
constexpr R extractFunctionReturn(R (T::*)(Args...) const)
{
    return R(); 
}

template <typename F>
constexpr auto extractArgs(F&& f)
{
    return extractFunctionArgs(&std::decay_t<F>::operator());
}

template <typename F>
constexpr auto extractReturn(F&& f)
{
    return extractFunctionReturn(&std::decay_t<F>::operator());
}

template <typename F>
constexpr auto makeComponent(F&& f)
{
    using producer_tuple = decltype(extractReturn(f));
    using consumer_tuple = decltype(extractArgs(f));
    using producer_type = decltype(makeProducerType(producer_tuple()));
    return makeComponentType<producer_type>(consumer_tuple(), f);
}

template <typename F>
constexpr auto makeConsumer(F&& f)
{
    using consumer_tuple = decltype(extractArgs(f));
    return makeConsumerType(consumer_tuple(), f);
}

int main()
{
    extractReturn([]{return 1;});

    auto head = makeComponent([](const int number){ std::cout << "Here it is in head: " << number << '\n'; return std::make_tuple(number + 1);});
    auto next = makeConsumer([](const int number){ std::cout << "Here it is in next: " << number << '\n'; });

    // building the pipeline
    head.setConsumer(next);
    head.consume(1);

    return 0;
}
