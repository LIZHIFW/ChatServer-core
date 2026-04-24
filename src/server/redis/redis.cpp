#include "redis.hpp"
#include <iostream>
using namespace std;

Redis::Redis()
:_publish_context(nullptr)
,_subcribe_context(nullptr)
{}

Redis::~Redis()
{
    if(_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if(_subcribe_context != nullptr)
    {
        redisFree(_subcribe_context);
    }
}

bool Redis::connect()
{
    //负责publish发布消息上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);
    if(nullptr == _publish_context)
    {
        cerr<<"connect redis failes!"<<endl;
        return false;
    }

    //立即执行AUTH命令验证密码
    const char*redis_password = "123456";
    redisReply*reply = (redisReply*)redisCommand(_publish_context,"AUTH %s",redis_password);
    if(reply == nullptr)
    {
        cerr<<"密码验证失败:"<<_publish_context->errstr<<endl;
        freeReplyObject(reply);
        redisFree(_publish_context);
        exit(-1);
    }


    //负责subscribe订阅消息的上下文连接
    _subcribe_context = redisConnect("127.0.0.1",6379);
    if(nullptr == _subcribe_context)
    {
        cerr<<"connect redis failes!"<<endl;
        return false;
    }

    //立即执行auth命令 验证密码
    const char*redis_password2 = "123456";
    redisReply*reply2=(redisReply*)redisCommand(_subcribe_context,"auth %s",redis_password2);
    if(nullptr == reply2)
    {
        cerr<<"密码验证失败:"<<_subcribe_context->errstr<<endl;
        freeReplyObject(reply2);
        redisFree(_subcribe_context);
        exit(-1);
    }

    //在单独的线程中，监听通道上的事件，有消息给业务层进行上报
    thread t([&]()
    {
        observer_channel_message();
    });
    t.detach();
    cout<<"connect redis-server success!"<<endl;

    return true;
}

//向redis指定的通道Channel发布消息
bool Redis::publish(int channel,string message)
{
    redisReply*reply = (redisReply*)redisCommand(_publish_context,"publish %d %s",channel,message.c_str());
    if(nullptr == reply)
    {
        cerr<<"publish command failed!"<<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}

//向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
    /**
     * subscribe命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接受通道消息
     * 通道消息的接收专门在observe_channel_message函数中的独立线程中进行
     * 只负责发送命令，不阻塞接受redis server 响应消息，否则和notifyMsg线程抢占响应资源
     */
    if(REDIS_ERR == redisAppendCommand(this->_subcribe_context,"SUBSCRIBE %d",channel))
    {
        cerr<<"subscribe command filed!"<<endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subcribe_context,&done))
        {
            cerr<<"subscribe command failed!"<<endl;
            return false;
        }
    }
    return true;
}

//向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel)
{
    if(REDIS_ERR == redisAppendCommand(this->_subcribe_context,"unsubscribe %d",channel))
    {
        cerr<<"unsubscribe command faile!"<<endl;
        return false;
    }
    //redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕（done被置为1）
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subcribe_context,&done))
        {
            cerr<<"unsubscribe command failed!"<<endl;
            return false;
        }
    }
    return true;
}

//在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
    redisReply*repley = nullptr;

    while(REDIS_OK == redisGetReply(this->_subcribe_context,(void**)&repley))
    {
        //订阅收到的消息是一个带三元素的数组
        if(repley != nullptr && repley->element[2] != nullptr&&repley->element[2]->str!=nullptr)
        {
            //给业务层上报通道上 发送的消息
            _notify_message_handler(atoi(repley->element[1]->str),repley->element[2]->str);
        }
        freeReplyObject(repley);
    }
    cerr<<">>>>>>>>>>>>>>>>>>>>>>>observer_channel_message quit<<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;
}

void Redis::init_notify_handler(function<void(int,string)> fn)
{
    this->_notify_message_handler = fn;
}