#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

#include <mutex>
#include <memory>
#include <condition_variable>
#include <iostream>

template <typename T>
class ThreadsafeQueue
{

private:
    
    struct node
    {
        std::unique_ptr<node> next;
        std::unique_ptr<T> data;
    };
    
    mutable std::condition_variable _cond_variable;
    mutable std::mutex _head_mutex;
    mutable std::mutex _tail_mutex;
    std::unique_ptr<node> _head_node;
    node* _tail_node;

    node* getTail () const
    {
        std::scoped_lock lock (_tail_mutex);
        return _tail_node;
    }

    std::unique_ptr<node> popHead ()
    {
        std::unique_ptr<node> old_head = std::move(_head_node);
        _head_node = std::move(old_head->next);

        return old_head;
    }
    
    std::unique_lock<std::mutex> waitForData ()
    {
        std::unique_lock lock(_head_mutex);
        _cond_variable.wait(lock, [&]{ return _head_node.get() != getTail(); });

        return lock;
    }

public:

    ThreadsafeQueue ()
        : _head_node (std::make_unique<node>()), _tail_node(_head_node.get())
    {

    }

    ThreadsafeQueue (const ThreadsafeQueue& other) = delete;
    ThreadsafeQueue (const ThreadsafeQueue&& other) = delete;

    void push (T data)
    {
        std::unique_ptr<node> new_node (std::make_unique<node>());
        std::unique_ptr<T> new_data(std::make_unique<T>(std::move(data)));

        {

            std::scoped_lock lock (_tail_mutex);

            _tail_node->data = std::move(new_data);
            _tail_node->next = std::move(new_node);
            _tail_node = _tail_node->next.get();

        }

        _cond_variable.notify_one();
    }


    std::unique_ptr<T> pop ()
    {
        std::unique_lock lock (_head_mutex);

        if (_head_node.get() == getTail())
        {
            return std::unique_ptr<T>();
        }

        std::unique_ptr<node> old_node = popHead();
        lock.unlock();

        return std::move(old_node->data);
    }

    bool pop (T& value)
    {
        std::scoped_lock lock (_head_mutex);

        if (_head_node.get() == getTail())
        {
            return false;
        }

        value = std::move(*(_head_node->data));
        std::unique_ptr<node> old_node = popHead();

        return true;
    }

    std::unique_ptr<T> waitPop ()
    {
        std::unique_lock lock = waitForData();

        std::unique_ptr<node> old_node = popHead();
        lock.unlock();

        return std::move(old_node->data);
    }

    void waitPop (T& value)
    {
        std::unique_lock lock = waitForData();

        value = std::move(*(_head_node->data));
        std::unique_ptr<node> old_head_node = popHead();
        lock.unlock();

    }

};



#endif // THREADSAFE_QUEUE_H