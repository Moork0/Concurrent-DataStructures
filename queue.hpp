#ifndef THREADSAFE_QUEUE_H
#define THREADSAFE_QUEUE_H

#include <mutex>
#include <memory>


template <typename T>
class ThreadsafeQueue
{

private:
    
    struct node
    {
        std::unique_ptr<node> next;
        std::unique_ptr<T> data;
    };
    
    mutable std::mutex _head_mutex;
    mutable std::mutex _tail_mutex;
    std::unique_ptr<node> _head_node;
    node* _tail_node;

    node* getTail () const
    {
        std::scoped_lock lock (_tail_mutex);
        return _tail_node;
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

        std::scoped_lock lock (_tail_mutex);

        _tail_node->data = std::move(new_data);
        _tail_node->next = std::move(new_node);
        _tail_node = _tail_node->next.get();
    }


    std::unique_ptr<T> pop ()
    {
        std::unique_lock lock (_head_mutex);

        if (_head_node.get() == getTail())
        {
            return std::unique_ptr<T>();
        }

        std::unique_ptr<node> old_node = std::move(_head_node);
        _head_node = std::move(old_node->next);

        lock.unlock();

        return std::move(old_node->data);
    }

};



#endif // THREADSAFE_QUEUE_H