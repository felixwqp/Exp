//
// Created by 王起鹏 on 12/12/21.
//

#ifndef PLAYGROUND_RATELIMITER_H
#define PLAYGROUND_RATELIMITER_H
#include <iostream>
#include <chrono>
/*
 * Rate Limiter
 * Problem: noisy neighbor problem
 *
 * Requirement:
 *
 * 1. Auto-scaling?
 *      takes time, still need rate limiter.
 *      before scale, without limit, already crash
 * 2. load balancer?
 *      1. indiscrimination: some request long/
 *      2. on application server, rather than load balancer level
 *
 * functional
 * 1. bool allowRequest(request)
 *
 * Non-Functional:
 * 1. low-latency: make decision quick
 * 2. accurate:
 * 3. scalable:
 *
 * ease of integration -> pattern
 *
 * */


/*Single Server*/

class TokenBucket{
private:
    const int maxToken_;
    int curToken_;
    int rateRefill_;
    std::chrono::time_point<std::chrono::system_clock> lastRefillTime_;
    void refill(){
        auto now = std::chrono::system_clock::now();
        int time_duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastRefillTime_).count();
        curToken_ = std::min(curToken_ + time_duration / 1000 * rateRefill_, maxToken_);

        lastRefillTime_ = now;
    }
public:
    TokenBucket(int maxToken, int rateRefill)
        : maxToken_(maxToken),
          rateRefill_(rateRefill),
          curToken_(maxToken){
        lastRefillTime_ = std::chrono::system_clock::now();
    }

    bool allowRequest(int tokens){
        refill();
        if(curToken_ >= tokens){
            curToken_ -= tokens;
            return true;
        }
        return false;
    }
};


class RateLimitter
{
private:
    queue<steady_clock::time_point> requests_;
    milliseconds interval_;
    unsigned int requestLimit_;

public:
    RateLimitter(milliseconds interval, unsigned int requestLimit)
            : interval_(interval), requestLimit_(requestLimit) {}

    bool processRequest() {
        auto now = steady_clock::now();
        if (!requests_.empty() && (now - requests_.front()) > interval_) requests_.pop();
        if (requests_.size() < requestLimit_) {
            requests_.push(now);
            return true;
        }
        return false;
    }
};



/*Interface*/





#endif //PLAYGROUND_RATELIMITER_H
