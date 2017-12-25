LigthingIO is a 'light' net work IO application framework;

learning and integrate them into LightingIO

因为转入后台开发不久, 刚开始想借助于现有的网络io库来构建一个服务应用框架而已,于是在调研和了解了libuv libev libevent之后, 选择了libevent,
一开始思考着他自带的协议支持和相对完备的实现, 可是渐渐的发现使用别人的库并不是一种好的体验, 他严重的制约了自己所想要的实现;

于是,我抛弃了我之前写的东西, 在这个基础之上,开始从epoll,eventloop, messageloop, dispatcher protocol decode/encode来构建, 变成我想要它成为的那个样子
