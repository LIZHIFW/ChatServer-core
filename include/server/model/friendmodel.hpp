#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H

#include "user.hpp"
#include<vector>

//维护好友信息的操作接口方法
class FriendModel
{
public:
    //添加好友关系
    void insert(int userid,int frinedid);

    //返回好友列表
    std::vector<User> query(int userid);
};


#endif