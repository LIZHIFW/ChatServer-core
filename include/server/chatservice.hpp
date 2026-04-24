#ifndef CHATSERVICE_H
#define CHARSERVICE_H

#include<muduo/net/TcpConnection.h>
#include"offlinemessagemodel.hpp"
#include "redis.hpp"
#include "groupmodel.hpp"
#include "friendmodel.hpp"
#include"json.hpp"
#include"usermodel.hpp"
#include<unordered_map>
#include<functional>
#include<mutex>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json=nlohmann::json;

//表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr&conn,json &js,Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
        //获取单例对象的接口函数
        static ChatService*instance();
        //处理登录业务
        void login(const TcpConnectionPtr&conn,json& js,Timestamp time);
        //处理注册业务
        void reg(const TcpConnectionPtr& conn,json&js,Timestamp time);
        //一对一聊天业务
        void oneChat(const TcpConnectionPtr& conn,json&js,Timestamp time);
        //添加好友业务
        void addFriend(const TcpConnectionPtr&conn,json&js,Timestamp time);
        //处理客户端的异常退出
        void clientCloseException(const TcpConnectionPtr& conn);

        //创建群组业务
        void createGroup(const TcpConnectionPtr&conn,json&js,Timestamp time);

        //加入群组业务
        void addGroup(const TcpConnectionPtr& conn,json&js,Timestamp time);

        //群组聊天业务
        void groupChat(const TcpConnectionPtr& conn,json&js,Timestamp time);
        //处理客户端的异常退出
        void reset();

        //获取消息对应的处理器
        MsgHandler getHandler(int magid);

        //从redis消息队列中获取订阅的消息
        void handlerRedisSubscribeMessage(int,string);

private:
    ChatService();
    
    //

    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler>_msgHandlerMap;
    //存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr>_userConnMap;

    mutex _connMutex;
    //数据操作类对象
    UserModel _userModel;

    offlineMessageModel _offlineMsgModel;

    FriendModel _friendModel;

    GroupModel _groupModel;

    Redis _redis;


};






#endif