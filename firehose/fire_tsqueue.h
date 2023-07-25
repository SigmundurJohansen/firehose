#pragma once
#include "firehose.h"


namespace firehose
{
	template<typename T>
	class TSQueue
	{
	public:
		TSQueue() = default;
		TSQueue(const TSQueue<T>&) = delete;
		virtual ~TSQueue() { clear(); }
	public:
		const T& front()
		{
			std::scoped_lock lock(m_queue);
			return m_dequeue.front();
		}
		const T& back()
		{
			std::scoped_lock lock(m_queue);
			return m_dequeue.back();
		}
		T pop_front()
		{
			std::scoped_lock lock(m_queue);
			auto t = std::move(m_dequeue.front());
			m_dequeue.pop_front();
			return t;
		}
		T pop_back()
		{
			std::scoped_lock lock(m_queue);
			auto t = std::move(m_dequeue.back());
			m_dequeue.pop_back();
			return t;
		}
		void push_back(const T& item)
		{
			std::scoped_lock lock(m_queue);
			m_dequeue.emplace_back(std::move(item));

			std::unique_lock<std::mutex> ul(m_blocking);
			cvBlocking.notify_one();
		}
		void push_front(const T& item)
		{
			std::scoped_lock lock(m_queue);
			m_dequeue.emplace_front(std::move(item));

			std::unique_lock<std::mutex> ul(m_blocking);
			cvBlocking.notify_one();
		}

		bool empty()
		{
			std::scoped_lock lock(m_queue);
			return m_dequeue.empty();
		}
		size_t count()
		{
			std::scoped_lock lock(m_queue);
			return m_dequeue.size();
		}
		void clear()
		{
			std::scoped_lock lock(m_queue);
			return m_dequeue.clear();
		}
		void wait()
		{
			while (empty())
			{
				std::unique_lock<std::mutex> ul(m_blocking);
				cvBlocking.wait(ul);
			}
		}

	protected:
		std::mutex m_queue;
		std::mutex m_blocking;
		std::deque<T> m_dequeue;
		std::condition_variable cvBlocking;
	};


}