#include "widget.h"
#include "ui_widget.h"
#include <iostream>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);

    init();
    initCommand();
}

Widget::~Widget()
{
    delete ui;
}

void Widget::timeoutUpdate()
{
    //帧同步 掉线之后，无法加入，
    //因为 帧同步记录的是玩家的操作， 所以掉线之后，当前帧的ID已经改变了， 无法知道其他玩家当前的状态； 除非把所有玩家的操作全部保存下来， 等玩家重连上来， 全部发过去;

    //客户端逻辑：
    //1.        判断当前帧F是否关键帧K1：如果不是跳转（7）。
    //2.        如果是关键帧，则察看有没有K1的UPDATE数据，如果没有的话重复2等待。
    //3.        采集当前K1的输入作为CTRL数据与K1编号一起发送给服务器
    //4.        从UPDATE K1中得到下一个关键帧的号码K2以及到下一个关键帧之间的输入数据I。
    //5.        从这个关键帧到下 一个关键帧K2之间的虚拟输入都用I。
    //6.        令K1 = K2。
    //7.        执行该帧逻辑
    //8.        跳转（1）
    //乐观帧锁定， 不等待
    //是关键帧

    if ( nextFrameId == curFrameId )
    {
        MapServerFrameInfo::iterator it = mFrameInfo.find(nextFrameId);
        if (it != mFrameInfo.end() )
        {
            //触发上报逻辑
            updateKeyInfoToServer();
            //得到下一帧的ID
            nextFrameId = it->second.nextFrameId;
            //执行操作的逻辑, 根据操作更新每个玩家信息

            std::map<uint32, VeFrameInfo>
            for (auto v : it->second.mUidFrameInfo)
            {
                if ( v.first == play.uid )
                {
                    play.x
                }
            }
        }
        else
        {
            //return等待数据
            return;
        }

    }
    curFrameId++;
    update();
}

void Widget::timeoutHeartbeat()
{
    if ( tcpSocket->state() != QAbstractSocket::ConnectedState ) return;
    cs::C2S_Ping ping;

    QDateTime time = QDateTime::currentDateTime();
    QTime current_time = QTime::currentTime();
    uint32 timems = current_time.msec();

    ping.set_send_time(timems);

    std::string msg;
    msg = ping.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Ping), msg);
}

void Widget::initCommand()
{
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Login, handlerLogin);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Move, handlerMove);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_AllPos, handlerUpdateAllUsers);
    REGISTER_CMD_CALLBACK(cs::ID_S2C_Ping, handlerHeartbeat);
}

void Widget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);

    // 反走样
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 设置画笔颜色、宽度
    painter.setPen(QPen(QColor(0, 160, 230), 2));

    painter.drawRect(QRect(mapxBegin,mapyBegin, mapxEnd, mapyEnd));
    QBrush brush(Qt::black);
    painter.setBrush(brush);
    painter.drawEllipse(QPointF(play.x, play.y), 5, 5);

    for (auto item : play.mSights)
    {
        painter.drawEllipse(QPointF(item.second.x, item.second.y), 5, 5);
    }

//    static QTime time;
//    static int frames = 0;
//    static bool started = false;

//    if (!started || time.elapsed() > 1000)
//    {
//        qreal fps = frames * 1000. / time.elapsed();
//        if (fps == 0)
//            m_current_fps = "fps ";
//        else
//            m_current_fps = QString::fromLatin1("fps %3").arg((int) qRound(fps));

//        time.start();
//        started = true;
//        frames = 0;
//    }
//    else
//    {
//        ++frames;
//        painter.setPen(QPen(QColor(52, 233, 48), 2));
//        painter.setFont(QFont("times", 14));
//        painter.drawText(width() - 100, 30, m_current_fps);
//        //sklog_info("m_current_fps : %s", m_current_fps.toStdString().c_str());

//        painter.setFont(QFont("rtt", 14));
//        painter.drawText(width() - 100, 50, m_current_rtt);
//    }

}

void Widget::mousePressEvent(QMouseEvent *e)
{

}

void Widget::keyPressEvent(QKeyEvent *e)
{
    int step = 5;
    int& x = play.x;
    int& y = play.y;

    std::unique_lock<std::mutex> lck(mtx);

    switch (e->key())
    {
        case Qt::Key_W :
        {
            //y = y-step; playerMove(x, y);
            vCurFrameInfo.push_back(E_PLAYER_KEY_UP);
            break;
        }
        case Qt::Key_S :
        {
            //y = y+step; playerMove(x, y);
            vCurFrameInfo.push_back(E_PLAYER_KEY_DOWN);
            break;
        }
        case Qt::Key_A :
        {
            //x = x-step; playerMove(x, y);
            vCurFrameInfo.push_back(E_PLAYER_KEY_LEFT);
            break;
        }
        case Qt::Key_D :
        {
            //x = x+step; playerMove(x, y);
            vCurFrameInfo.push_back(E_PLAYER_KEY_RIGHT);
            break;
        }
        //connect server
        case Qt::Key_Space : connectServer(); break;
    }
}

void Widget::init()
{
    /////// map is 500 * 500
    mapxBegin = 10;
    mapyBegin = 10;
    mapxEnd = mapxBegin + 500;
    mapyEnd = mapyBegin + 500;
    play.x = mapxBegin;
    play.y = mapyBegin;
    this->setFixedSize(mapxEnd + 20, mapyEnd + 20);
    /////////network/////////
    this->tcpSocket = new QTcpSocket(this);

    connect(tcpSocket,SIGNAL(readyRead()),this,SLOT(revData()));

    connect(tcpSocket,SIGNAL(error(QAbstractSocket::SocketError)),
               this,SLOT(displayError(QAbstractSocket::SocketError)));

    ////////heartbeat/////////
    m_current_rtt = "rtt -1ms";
    m_heartbeat = new QTimer;
    connect(m_heartbeat,SIGNAL(timeout()),this,SLOT(timeoutHeartbeat()));
    m_heartbeat->start(10000);//10s 心跳

    /////////frame//////// 1s  50  20 * 5= 100 -> 10
//    sumFrameAdd = 0;
//    upStep = 5;
//    curFrameId = sumFrameAdd + upStep;
}

void Widget::connectServer()
{
    tcpSocket->abort();
    tcpSocket->connectToHost("192.168.119.129",2007);
    klog_info("connect server");
    playerLogin();
}

void Widget::revData()
{
   QByteArray datas = tcpSocket->readAll();

   buf_.append(datas.data(), datas.size());

   for (;;)
   {
       if ( buf_.size() < PACKET_HEAD_LEN )
       {
           break;
       }

       uint32 msgLen = buf_.peekUint32();
       if ( msgLen > buf_.size() ) {
         klog_info(">>>>>>> MSG LEN : %d | buf len : %d", msgLen, buf_.size());
         break;
       }

       struct PACKET pkt;
       pkt.len = buf_.readInt32();
       pkt.cmd = buf_.readInt32();
       pkt.uid = buf_.readInt32();
       pkt.msg = std::string(buf_.retrieveBuf(pkt.len - PACKET_HEAD_LEN));

       if ( command_.find(pkt.cmd) != command_.end() ) {
            command_[pkt.cmd](pkt.msg);
       }
   }
   //klog_info("BUFFER size : %d", buf_.size());
   //klog_info("Proto Callback Not Found This Func : %d", pkt.cmd);
}


void Widget::displayError(QAbstractSocket::SocketError)
{
   qDebug()<<tcpSocket->errorString();

   tcpSocket->close();
}

void Widget::sendPacket(int cmdId, std::string& msg)
{
    struct PACKET pkt;

    pkt.len = PACKET_HEAD_LEN + msg.size();
    pkt.cmd = cmdId;
    pkt.uid = play.uid;
    pkt.msg = msg;

    char* msg_ = new char[pkt.len];
    memset(msg_, 0, pkt.len);
    memcpy(msg_, (char*)&(pkt.len), sizeof(uint32));
    memcpy(msg_ + 4, (char*)&pkt.cmd, sizeof(uint32));
    memcpy(msg_ + 8, (char*)&pkt.uid, sizeof(uint32));
    memcpy(msg_ + 12, pkt.msg.c_str(), msg.size());

    tcpSocket->write(msg_, pkt.len);
    //klog_info("cmdId : %d | msg len : %d | sum : %d", pkt.cmd, msg.size(), pkt.len);
    delete msg_;
}

void Widget::updateKeyInfoToServer()
{
    cs::C2S_Frame frame;
    uint32* uFrame;
    frame.set_uid(play.uid);
    frame.set_frameId(curFrameId);

    std::unique_lock<std::mutex> lck(mtx);
    for (std::vector<uint32>::iterator it = vCurFrameInfo.begin(); it != vCurFrameInfo.end(); ++it )
    {
        frame.add_keyInfo(*it);
    }
    vCurFrameInfo.clear();

    std::string msg;
    msg = frame.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Frame), msg);
}

/////////////////////////
void Widget::playerLogin()
{
    cs::C2S_Login login;

    std::string msg;
    msg = login.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Login), msg);
}

void Widget::playerMove(int x, int y)
{
    cs::C2S_Move move;
    move.set_dwx(x);
    move.set_dwy(y);

    std::string msg;
    msg = move.SerializeAsString();
    sendPacket(uint32(cs::ProtoID::ID_C2S_Move), msg);
}

void Widget::playerAttack()
{

}

////////////////////
bool Widget::handlerLogin(std::string& str)
{
    cs::S2C_Login login;
    if ( !login.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    //klog_info("handler login");

    if ( play.uid ==  0 )
    {
        //klog_info("play.uid : %d, newId : %d", play.uid, login.uid());
        play.uid = login.uid();
    }

    return true;
}

bool Widget::handlerMove(std::string& str)
{
    cs::S2C_Move move;
    if ( !move.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    //klog_info("handlerMove");

    uint32 uid = move.dwuid();
    uint32 dwx = move.dwx();
    uint32 dwy = move.dwy();

    // 如果是自己, 则更新自己即可
    if ( play.uid == uid )
    {
        play.x = dwx;
        play.y = dwy;
        update();
        //klog_info("Self Change Pos %d %d", play.x, play.y);
    }

    return true;
}

bool Widget::handlerUpdateAllUsers(std::string& str)
{
    cs::S2C_AllPos pos;
    if ( !pos.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    int size = pos.users_size();
    for (int i = 0; i < size; i++)
    {
        cs::User user = pos.users(i);
        struct Player p;
        p.x = user.dwx();
        p.y = user.dwy();
        p.uid = user.uid();

        if ( user.uid() == play.uid)
        {
            play.x = p.x;
            play.y = p.y;
            continue;
        }

        struct Player& otherPlay = play.mSights.insert(std::make_pair(p.uid, p)).first->second;
        otherPlay.x = p.x;
        otherPlay.y = p.y;
    }

    update();
    //klog_info(" sight : %d", play.mSights.size());
    return true;
}

bool Widget::handlerHeartbeat(std::string& str)
{
    cs::S2C_Ping sping;

    if ( !sping.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    QDateTime time = QDateTime::currentDateTime();
    QTime current_time = QTime::currentTime();
    uint32 timems = current_time.msec();

    uint32 sendtime = sping.send_time();
    qreal rtt = qreal(timems - sendtime);

    m_current_rtt = QString::fromLatin1("rtt %3 ms").arg((int) qRound(rtt));
}

bool Widget::handlerFrameInit(std::string& str)
{
    cs::S2C_FrameInit frameinit;

    if ( !frameinit.ParseFromString(str) )
    {
        klog_info("eroor parse proto");
        return false;
    }

    curFrameId = frameinit.curframeid();
    nextFrameId = frameinit.nextframeid();
    ////////update////////////
    m_timer = new QTimer;
    connect(m_timer,SIGNAL(timeout()),this,SLOT(timeoutUpdate()));
    m_timer->start(10); //60帧 50ms 上报一次
}
