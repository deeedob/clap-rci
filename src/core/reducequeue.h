#ifndef REDUCEQUEUE_H
#define REDUCEQUEUE_H

#include "global.h"

#include <functional>
#include <unordered_map>
#include <atomic>
#include <array>
#include <concepts>
#include <cassert>

RCLAP_BEGIN_NAMESPACE

template <typename Target, typename Other>
concept UpdatableValue = requires(Target &lhs, const Other &rhs) {
    { lhs.update(rhs) };
};

template <typename Target, typename Other>
concept FromOther = requires(Target &lhs, const Other &rhs) {
    { lhs(rhs) };
};

template <typename Key, typename Value>
class ReduceQueue
{
public:
    using QueueType = std::unordered_map<Key, Value>;
    using ConsumeType = std::function<void (const Key &key, const Value &value)>;

    ReduceQueue()
    {
        reset();
    }

    void setCapacity(std::size_t capacity)
    {
        for (auto &q : queues)
            q.reserve(2 * capacity);
    }

    void set(const Key &key, const Value &value)
    {
        producer.load()->insert_or_assign(key, value);
    }

    template <typename Other>
    void setOrUpdate(const Key &key, const Other &value) requires UpdatableValue<Value, Other>
    {
        auto it = producer.load()->try_emplace(key, Value(value));
        if (!it.second)
            it.first->second.update(value);
    }

    void producerDone()
    {
        if (consumer)
            return;
        auto tmp = producer.load();
        producer = free.load();
        free = nullptr;
        consumer = tmp;
        assert(producer);
    }

    [[nodiscard]] size_t size() const
    {
        if (!consumer)
            return 0;
        return consumer.load()->size();
    }

    void consume(const ConsumeType &consume)
    {
        assert(consume);

        if (!consumer)
            return;

        for (auto &kv : *consumer)
            consume(kv.first, kv.second);

        consumer.load()->clear();
        if (free)
            return;

        free = consumer.load();
        consumer = nullptr;
    }

    void reset()
    {
        for (auto &q : queues)
            q.clear();
        free = &queues.at(0);
        producer = &queues.at(0);
        consumer = nullptr;
    }

private:
    std::array<QueueType, 2> queues;
    std::atomic<QueueType*> free;
    std::atomic<QueueType*> producer;
    std::atomic<QueueType*> consumer;
};

RCLAP_END_NAMESPACE

#endif // REDUCEQUEUE_H
