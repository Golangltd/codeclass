//package main

//import (
//	"bufio"
//	"encoding/json"
//	"flag"
//	"fmt"
//	"glog-master" // 此包：Golang语言社区资源站下载，www.Golang.MoM
//	"log"
//	"math"
//	"net"
//	"net/http"
//	"net/url"
//	"os"
//	"strings"
//	"sync"
//	"time"
//)

//type Abser interface {
//	Abs() float64
//}

//func main333() {
//	var a Abser
//	f := MyFloat(-math.Sqrt2)
//	v := Vertex{3, 4}
//	a = f
//	fmt.Println(a.Abs())
//	a = &v
//	// implement Abser
//	fmt.Println(a.Abs())
//}

//type MyFloat float64

//func (f MyFloat) Abs() float64 {
//	if f < 0 {
//		return float64(-f)
//	}
//	return float64(f)
//}

//type Vertex struct {
//	X, Y float64
//}

//func (v *Vertex) Abs() float64 {
//	return math.Sqrt(v.X*v.X + v.Y*v.Y)
//}

//func init() {
//	//go tetsAPI()

//	str := " li hiabin "
//	fmt.Println(str)

//	req := "http://api1.yulegame.cn:9000/API/SetGameData.ashx?method=InsertData&OpenID=12345625&NickName=" + str + "&Sex=1&Language=zh_CN&City=&Province=&Country=%E6%88%90%E9%83%BD&Headimgurl=&IdentifykeyHD=&GameName=%E5%A4%A7%E8%88%AA%E6%B5%B7&GameType=3&StoreName=admin001"
//	glog.Info(req)

//	u, _ := url.Parse(req)
//	q := u.Query()
//	u.RawQuery = q.Encode() //urlencode
//	res, err := http.Get(u.String())

//	glog.Info(res)
//	glog.Info(err)

//	// 初始化 日志系统
//	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
//	flag.Set("log_dir", "./log")        // 日志文件保存目录
//	flag.Set("v", "3")                  // 配置V输出的等级。
//	flag.Parse()

//	return
//}

//type product struct {
//	Productid   string
//	Productname string
//	Unitcost    float32
//	Status      string
//	Listprice   float32
//	Attr1       string
//	Itemid      string
//}

//type grid struct {
//	Total int
//	Rows  []product
//}

////连接服务器
//func connectServer() {
//	//接通
//	//	go tetsAPI()
//	conn, err := net.Dial("tcp", "localhost:8888")
//	checkError(err)
//	fmt.Println("连接成功！\n")
//	//输入
//	inputReader := bufio.NewReader(os.Stdin)
//	fmt.Println("你是谁？")
//	//	name, _ := inputReader.ReadString('\n')

//	DATA := &product{
//		Productid: "2",
//	}
//	dd, _ := json.Marshal(DATA)
//	//
//	trimName := strings.Trim(string(dd), "\r\n")
//	fmt.Println(dd)
//	conn.Write(dd)
//	for {
//		fmt.Println("我们来聊天吧！按quit退出")
//		//读一行
//		input, _ := inputReader.ReadString('\n')
//		trimInput := strings.Trim(input, "\r\n")
//		//如果quit就退出
//		if trimInput == "quit" {
//			fmt.Println("再见")
//			conn.Write([]byte(trimName + " 退出了 "))
//			return
//		}
//		//写出来
//		_, err = conn.Write([]byte(trimName + " says " + trimInput))
//	}
//}

////检查错误
//func checkError(err error) {
//	if err != nil {
//		log.Fatal("an error!", err.Error())
//	}
//}

////主函数
//func main() {
//	//连接servser
//	connectServer()
//}

//func main11() {

//	// 客户端结构

//	//	msg := &MsgParser{
//	//		lenMsgLen: 1,
//	//		minMsgLen: 1,
//	//		maxMsgLen: 1000,
//	//		//		LittleEndian bool
//	//	}

//	//	agent := &TCPConn{
//	//		//sync.Mutex
//	//		//conn      net.Conn
//	//		//writeChan chan []byte
//	//		//closeFlag bool
//	//		msgParser: msg,
//	//	}

//	clent := &TCPClient{
//		//		sync.Mutex
//		Addr:    "127.0.0.1:8888",
//		ConnNum: 10,
//		//		ConnectInterval time.Duration
//		//		PendingWriteNum int
//		AutoReconnect: true,
//		//NewAgent        :func(agent) Agent,
//		//		conns           ConnSet
//		//		wg              sync.WaitGroup
//		//		closeFlag       bool

//		// msg parser
//		LenMsgLen: 1,
//		MinMsgLen: 1,
//		MaxMsgLen: 1000,
//		//		LittleEndian bool
//		//		msgParser    *MsgParser
//	}

//	clent.Start()

//	return
//}

////-----------------------------------------------------------------------------

//type ConnSet map[net.Conn]struct{}

//type TCPConn struct {
//	sync.Mutex
//	conn      net.Conn
//	writeChan chan []byte
//	closeFlag bool
//	msgParser *MsgParser
//}

//// --------------
//// | len | data |
//// --------------
//// 数据结构信息
//type MsgParser struct {
//	lenMsgLen    int
//	minMsgLen    uint32
//	maxMsgLen    uint32
//	littleEndian bool
//}

//// 接口信息
//type Agent interface {
//	Run()
//	OnClose()
//}

//// 客户端结构
//type TCPClient struct {
//	sync.Mutex
//	Addr            string
//	ConnNum         int
//	ConnectInterval time.Duration
//	PendingWriteNum int
//	AutoReconnect   bool
//	NewAgent        func(*TCPConn) Agent
//	conns           ConnSet
//	wg              sync.WaitGroup
//	closeFlag       bool

//	// msg parser
//	LenMsgLen    int
//	MinMsgLen    uint32
//	MaxMsgLen    uint32
//	LittleEndian bool
//	msgParser    *MsgParser
//}

//func NewMsgParser() *MsgParser {
//	p := new(MsgParser)
//	p.lenMsgLen = 2
//	p.minMsgLen = 1
//	p.maxMsgLen = 4096
//	p.littleEndian = false

//	return p
//}

//func (client *TCPClient) Start() {
//	client.init()

//	for i := 0; i < client.ConnNum; i++ {
//		client.wg.Add(1)
//		go client.connect()
//	}
//}

//func (client *TCPClient) init() {
//	client.Lock()
//	defer client.Unlock()

//	if client.ConnNum <= 0 {
//		client.ConnNum = 1
//		glog.Info("invalid ConnNum, reset to %v", client.ConnNum)
//	}
//	if client.ConnectInterval <= 0 {
//		client.ConnectInterval = 3 * time.Second
//		glog.Info("invalid ConnectInterval, reset to %v", client.ConnectInterval)
//	}
//	if client.PendingWriteNum <= 0 {
//		client.PendingWriteNum = 100
//		glog.Info("invalid PendingWriteNum, reset to %v", client.PendingWriteNum)
//	}
//	//	if client.NewAgent == nil {
//	//		log.Fatal("NewAgent must not be nil")
//	//	}
//	if client.conns != nil {
//		log.Fatal("client is running")
//	}

//	client.conns = make(ConnSet)
//	client.closeFlag = false

//	// msg parser
//	msgParser := NewMsgParser()
//	msgParser.SetMsgLen(client.LenMsgLen, client.MinMsgLen, client.MaxMsgLen)
//	msgParser.SetByteOrder(client.LittleEndian)
//	client.msgParser = msgParser
//}

//// It's dangerous to call the method on reading or writing
//func (p *MsgParser) SetByteOrder(littleEndian bool) {
//	p.littleEndian = littleEndian
//}

//// It's dangerous to call the method on reading or writing
//func (p *MsgParser) SetMsgLen(lenMsgLen int, minMsgLen uint32, maxMsgLen uint32) {
//	if lenMsgLen == 1 || lenMsgLen == 2 || lenMsgLen == 4 {
//		p.lenMsgLen = lenMsgLen
//	}
//	if minMsgLen != 0 {
//		p.minMsgLen = minMsgLen
//	}
//	if maxMsgLen != 0 {
//		p.maxMsgLen = maxMsgLen
//	}

//	var max uint32
//	switch p.lenMsgLen {
//	case 1:
//		max = math.MaxUint8
//	case 2:
//		max = math.MaxUint16
//	case 4:
//		max = math.MaxUint32
//	}
//	if p.minMsgLen > max {
//		p.minMsgLen = max
//	}
//	if p.maxMsgLen > max {
//		p.maxMsgLen = max
//	}
//}

//func (client *TCPClient) dial() net.Conn {
//	for {
//		conn, err := net.Dial("tcp", client.Addr)
//		if err == nil || client.closeFlag {
//			return conn
//		}

//		glog.Info("connect to %v error: %v", client.Addr, err)
//		time.Sleep(client.ConnectInterval)
//		continue
//	}
//}

//func newTCPConn(conn net.Conn, pendingWriteNum int, msgParser *MsgParser) *TCPConn {
//	tcpConn := new(TCPConn)
//	tcpConn.conn = conn
//	tcpConn.writeChan = make(chan []byte, pendingWriteNum)
//	tcpConn.msgParser = msgParser

//	go func() {
//		for b := range tcpConn.writeChan {
//			if b == nil {
//				break
//			}

//			_, err := conn.Write(b)
//			if err != nil {
//				break
//			}
//		}

//		conn.Close()
//		tcpConn.Lock()
//		tcpConn.closeFlag = true
//		tcpConn.Unlock()
//	}()

//	return tcpConn
//}

//func (client *TCPClient) connect() {
//	defer client.wg.Done()

//reconnect:
//	conn := client.dial()
//	if conn == nil {
//		return
//	}

//	client.Lock()
//	if client.closeFlag {
//		client.Unlock()
//		conn.Close()
//		return
//	}
//	client.conns[conn] = struct{}{}
//	client.Unlock()

//	tcpConn := newTCPConn(conn, client.PendingWriteNum, client.msgParser)
//	agent := client.NewAgent(tcpConn)
//	agent.Run()

//	// cleanup
//	tcpConn.Close()
//	client.Lock()
//	delete(client.conns, conn)
//	client.Unlock()
//	agent.OnClose()

//	if client.AutoReconnect {
//		time.Sleep(client.ConnectInterval)
//		goto reconnect
//	}
//}

//func (tcpConn *TCPConn) Close() {
//	tcpConn.Lock()
//	defer tcpConn.Unlock()
//	if tcpConn.closeFlag {
//		return
//	}

//	tcpConn.doWrite(nil)
//	tcpConn.closeFlag = true
//}

//func (tcpConn *TCPConn) doWrite(b []byte) {
//	if len(tcpConn.writeChan) == cap(tcpConn.writeChan) {
//		glog.Info("close conn: channel full")
//		tcpConn.doDestroy()
//		return
//	}

//	tcpConn.writeChan <- b
//}

//func (tcpConn *TCPConn) doDestroy() {
//	tcpConn.conn.(*net.TCPConn).SetLinger(0)
//	tcpConn.conn.Close()

//	if !tcpConn.closeFlag {
//		close(tcpConn.writeChan)
//		tcpConn.closeFlag = true
//	}
//}

//func (client *TCPClient) Close() {
//	client.Lock()
//	client.closeFlag = true
//	for conn := range client.conns {
//		conn.Close()
//	}
//	client.conns = nil
//	client.Unlock()

//	client.wg.Wait()
//}

//// 排序
//package main

//import (
//	"fmt"
//	"math/rand"
//	"runtime"
//	"strconv"
//	"time"
//)

//// 结构定义
//type Cserli struct {
//	Id   int
//	Name string
//}

//// 初始化函数
//func init() {
//	runtime.GOMAXPROCS(runtime.NumCPU() * 2)
//	t1 := time.Now()
//	//maptmp := make(map[int]*Cserli)
//	var list []Cserli
//	for i := 1; i < 100000; i++ {
//		txt := Cserli{}
//		txt.Id = rand.Intn(100000)
//		txt.Name = strconv.Itoa(i)
//		//maptmp[i] = txt
//		list = append(list, txt)
//	}
//	//fmt.Println(maptmp)
//	//fmt.Println("list:---------", list)
//	chT := make(chan Cserli, len(list))
//	STquicksort(list, chT, 0, 0)
//	fmt.Println("-------------", chT)
//	//	chTbak := make(chan Cserli, len(list))
//	//	for v := range chT {
//	//		fmt.Println(v)
//	//		chTbak <- v
//	//	}
//	//	close(chTbak)

//	//	for v := range chTbak {
//	//		fmt.Println("-------------", v)
//	//	}
//	elapsed := time.Since(t1)
//	fmt.Println("App elapsed: ", elapsed)
//	return
//}

//// threads 线程标识创建线程的个数
//func STquicksort(nums []Cserli, ch chan Cserli, level int, threads int) {
//	level = level * 2
//	if len(nums) == 1 {
//		ch <- nums[0]
//		close(ch)
//		return
//	} //ch<-nums[0] 表示将nums[0] 数据写到ch通道中
//	if len(nums) == 0 {
//		close(ch)
//		return
//	}

//	// 分组 定义空的数组
//	less := make([]Cserli, 0) //
//	greater := make([]Cserli, 0)

//	// 获取第一个数据
//	left := nums[0]      //快速排序的轴
//	leftid := nums[0].Id //快速排序的轴
//	nums = nums[1:]

//	//从左向右扫描数据 大于轴的放到greater里小于的放到less中
//	for _, num_data := range nums {
//		switch {
//		case num_data.Id >= leftid:
//			less = append(less, num_data)
//		case num_data.Id < leftid:
//			greater = append(greater, num_data)
//		}
//	}

//	left_ch := make(chan Cserli, len(less))
//	right_ch := make(chan Cserli, len(greater))

//	if level <= threads {
//		go STquicksort(less, left_ch, level, threads) //分任务
//		go STquicksort(greater, right_ch, level, threads)
//	} else {
//		STquicksort(less, left_ch, level, threads)
//		STquicksort(greater, right_ch, level, threads)
//	}

//	//	fmt.Println("---------------less,", less)
//	//	fmt.Println("---------------greater,", greater)
//	//	//--------------------------------------------------------------
//	//	fmt.Println("---------------left_ch,", left_ch)
//	//	fmt.Println("---------------right_ch,", right_ch)
//	//	//--------------------------------------------------------------

//	//合并数据

//	for i := range left_ch {
//		ch <- i
//	}
//	ch <- left
//	for i := range right_ch {
//		ch <- i
//	}
//	close(ch)
//	return
//}

//// 主函数
//func main1() {

//}

//func main() {

//	t1 := time.Now()
//	//maptmp := make(map[int]*Cserli)
//	var list []Cserli
//	for i := 1; i < 100000; i++ {
//		txt := Cserli{}
//		txt.Id = rand.Intn(100000)
//		txt.Name = strconv.Itoa(i)
//		//maptmp[i] = txt
//		list = append(list, txt)
//	}

//	length := len(list)
//	for root := length/2 - 1; root >= 0; root-- {
//		sort(list, root, length)
//	} //第一次建立大顶堆
//	for i := length - 1; i >= 1; i-- {
//		list[0], list[i] = list[i], list[0]
//		sort(list, 0, i)
//	} //调整位置并建并从第一个root開始建堆.假设不明确为什么,大家多把图画几遍就应该明朗了
//	fmt.Println(list)

//	elapsed := time.Since(t1)
//	fmt.Println("App elapsed: ", elapsed)
//}
//func sort(list []Cserli, root, length int) {
//	for {
//		child := 2*root + 1
//		if child >= length {
//			break
//		}
//		if child+1 < length && list[child].Id > list[child+1].Id {
//			child++ //这里重点讲一下,就是调整堆的时候,以左右孩子为节点的堆可能也须要调整
//		}
//		if list[root].Id < list[child].Id {
//			return
//		}
//		list[root], list[child] = list[child], list[root]
//		root = child
//	}
//}

//package main

//import (
//	"fmt"
//	"io"
//	"log"
//	"net/http"
//	"time"
//)

//var mux map[string]func(http.ResponseWriter, *http.Request)

//// 主函数
//func main() {
//	server := http.Server{
//		Addr:        ":8080",
//		Handler:     &MyHandle{},
//		ReadTimeout: 6 * time.Second,
//	}
//	mux = make(map[string]func(http.ResponseWriter, *http.Request))
//	mux["/hello"] = hello
//	mux["/bye"] = bye
//	err := server.ListenAndServe()

//	if err != nil {
//		log.Fatal(err)
//	}
//}

//type MyHandle struct{}

//func (*MyHandle) ServeHTTP(w http.ResponseWriter, r *http.Request) {
//	//req.URL.Path[len(req.URL.Path)-1]
//	fmt.Println(string(r.URL.Path[len(r.URL.Path)-1]))
//	//	for _, v := range r.URL.Path {
//	//		fmt.Println("v:-- ", string(v))
//	//	}
//	if h, ok := mux[r.URL.String()]; ok {
//		h(w, r)
//	}
//	io.WriteString(w, "URL"+r.URL.String())
//}

//func hello(w http.ResponseWriter, r *http.Request) {

//	r.ParseForm()
//	param_userName, found1 := r.Form["u"]
//	param_password, found2 := r.Form["p"]

//	if !(found1 && found2) {
//		fmt.Fprint(w, "请勿非法访问")
//		return
//	}
//	io.WriteString(w, "hello 模块"+param_userName[0]+param_password[0])
//}

//func bye(w http.ResponseWriter, r *http.Request) {
//	io.WriteString(w, "bye 模块")
//}

package main

import (
	"flag"
	//	"fmt"
	"net/http"
	"sync"
)

func main() {
	host := flag.String("host", "127.0.0.1", "listen host")
	port := flag.String("port", "8080", "listen port")

	http.HandleFunc("/hello", Hello)

	err := http.ListenAndServe(*host+":"+*port, nil)

	if err != nil {
		panic(err)
	}
}

func Hello(w http.ResponseWriter, req *http.Request) {

	//	defer func() { // 必须要先声明defer，否则不能捕获到panic异常
	//		if err := recover(); err != nil {
	//			fmt.Println("%s", err)

	//		}
	//	}()
	w.Write([]byte("Hello World"))
	panic("test")
}

//package main

//import (
//	"fmt"
//	"io/ioutil"
//	"net/http"
//	//	"time"
//)

//func main() {
//	for {
//		response, _ := http.Get("http://localhost:8080/hello")
//		defer response.Body.Close()
//		body, _ := ioutil.ReadAll(response.Body)
//		fmt.Println(string(body))
//	}

//}
