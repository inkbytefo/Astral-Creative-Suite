#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

namespace AstralEngine {
	class ThreadPool {
	public:
		ThreadPool(size_t threads);
		~ThreadPool();

		template<class F, class... Args>
		auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

	private:
		std::vector<std::thread> m_workers;
		std::queue<std::function<void()>> m_tasks;

		std::mutex m_queueMutex;
		std::condition_variable m_condition;
		bool m_stop;
	};

	template<class F, class... Args>
	auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type> {
		using return_type = typename std::result_of<F(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);

		std::future<return_type> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(m_queueMutex);

			if (m_stop) {
				throw std::runtime_error("enqueue on stopped ThreadPool");
			}

			m_tasks.emplace([task]() { (*task)(); });
		}
		m_condition.notify_one();
		return res;
	}
}