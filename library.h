#ifndef CPPCHAN_LIBRARY_H
#define CPPCHAN_LIBRARY_H

#include <type_traits>
#include <variant>
#include <functional>
#include <condition_variable>
#include <queue>
#include <mutex>

namespace CppChan {
    template<typename... Args>
    struct HandlerType {
        using type = std::tuple<std::function<void(Args)>...>;
    };
    template<typename... Args>
    class Channel {
        static_assert(std::conjunction<std::is_move_constructible<Args>...>::value,
                      "All message types must be move constructible classes or structs");
    private:
        using UniqueLock = std::unique_lock<std::mutex>;
        using Mutex = std::mutex;
        using Message = std::variant<Args...>;
        using CV = std::condition_variable;
        using Queue = std::queue<Message>;

        static constexpr size_t size = sizeof...(Args);
        typename HandlerType<const Args&...>::type rx_handlers;

        void invoke(Message& message) {
            std::visit([this](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                std::get<std::function<void(const T&)>>(rx_handlers)(arg);
            }, message);
        }

    public:
        class TX;
        class RX {
            Queue queue;
            Mutex mtx;
            CV cv;
            Channel& channel;
            explicit RX(Channel& c): channel(c) {}

            friend class Channel;
            friend class TX;
        public:
            RX(const RX&) = delete;
            RX& operator=(const RX&) = delete;
            RX(RX&&) = delete;
            RX& operator=(RX&&) = delete;

            void process_next() {
                Message message;
                {
                    UniqueLock lock(mtx);
                    if (queue.empty()) {
                        cv.wait(lock, [&] { return !queue.empty(); });
                    }
                    message = queue.front();
                    queue.pop();
                }
                cv.notify_one();
                channel.invoke(message);
            }
        };
        friend class RX;

        class TX {
        private:
            RX& receiver;
            explicit TX(RX& r): receiver(r) {}
            friend class Channel;
        public:
            TX(const TX&) = delete;
            TX& operator=(const TX&) = delete;
            TX(TX&&) = delete;
            TX& operator=(TX&&) = delete;

            template<typename T>
            void send(const T& message) {
                {
                    UniqueLock lock(receiver.mtx);
                    receiver.queue.push(message);
                }
                receiver.cv.notify_one();
            }
        };

        RX receiver;
        TX transmitter;

        explicit Channel(std::function<void(const Args&)>... rx_handler_list):
                rx_handlers(std::make_tuple(rx_handler_list...)),
                receiver(RX{*this}),
                transmitter(TX{receiver}) {}
        Channel(const Channel&) = delete;
        Channel& operator=(const Channel&) = delete;
        Channel(const Channel&&) = delete;
        Channel& operator=(const Channel&&) = delete;
    };

}
#endif //CPPCHAN_LIBRARY_H
