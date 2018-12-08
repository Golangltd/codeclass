package main

//import (
//	"flag"
//	"glog-master"
//	"log"
//	"net/http"
//	//	"os"
//	"crypto/md5"
//	"encoding/hex"
//	"encoding/xml"
//	"fmt"
//	//	"io/ioutil"
//	"opencv-master"
//	"sort"
//	"strings"
//)

//func init() {
//	// 初始化 日志系统
//	flag.Set("alsologtostderr", "true") // 日志写入文件的同时，输出到stderr
//	flag.Set("log_dir", "./log")        // 日志文件保存目录
//	flag.Set("v", "3")                  // 配置V输出的等级。
//	flag.Parse()
//	//	_, err := os.Open("apiclient_cert.pem")
//	//	if err != nil {
//	//		glog.Info(err)
//	//	}
//	return
//}

//func main() {
//	glog.Info("Entry main!!!")
//	// http.HandleFunc("/Pay", WxpayCallback)
//	//err := http.ListenAndServeTLS(":6089", "apiclient_cert.pem", "apiclient_key.pem", nil)
//	http.HandleFunc("/Pay", TJWanJiaData)
//	err := http.ListenAndServe(":6089", nil)
//	if err != nil {
//		log.Fatal("ListenAndServeTLS:", err.Error())
//	}
//}

////微信支付签名验证函数
//func wxpayVerifySign(needVerifyM map[string]interface{}, sign string) bool {
//	signCalc := wxpayCalcSign(needVerifyM, "API_KEY")
//	glog.Info("计算出来的sign: %v", signCalc)
//	glog.Info("微信异步通知sign: %v", sign)
//	if sign == signCalc {
//		glog.Info("签名校验通过!")
//		return true
//	}
//	glog.Info("签名校验失败!")
//	return false
//}

//type WXPayNotifyReq struct {
//	Return_code    string `xml:"return_code"`
//	Return_msg     string `xml:"return_msg"`
//	Appid          string `xml:"appid"`
//	Mch_id         string `xml:"mch_id"`
//	Nonce          string `xml:"nonce_str"`
//	Sign           string `xml:"sign"`
//	Result_code    string `xml:"result_code"`
//	Openid         string `xml:"openid"`
//	Is_subscribe   string `xml:"is_subscribe"`
//	Trade_type     string `xml:"trade_type"`
//	Bank_type      string `xml:"bank_type"`
//	Total_fee      int    `xml:"total_fee"`
//	Fee_type       string `xml:"fee_type"`
//	Cash_fee       int    `xml:"cash_fee"`
//	Cash_fee_Type  string `xml:"cash_fee_type"`
//	Transaction_id string `xml:"transaction_id"`
//	Out_trade_no   string `xml:"out_trade_no"`
//	Attach         string `xml:"attach"`
//	Time_end       string `xml:"time_end"`
//}

//type WXPayNotifyResp struct {
//	Return_code string `xml:"return_code"`
//	Return_msg  string `xml:"return_msg"`
//}

//// 微信返回的数据
////<xml>
////<appid><![CDATA[wxba1ee3c9fb5ba4cd]]></appid>
////<attach><![CDATA[222]]></attach>
////<bank_type><![CDATA[CFT]]></bank_type>
////<cash_fee><![CDATA[1]]></cash_fee>
////<fee_type><![CDATA[CNY]]></fee_type>
////<is_subscribe><![CDATA[N]]></is_subscribe>
////<mch_id><![CDATA[1327865501]]></mch_id>
////<nonce_str><![CDATA[25igemy7l6pyt7010tosomt6zv73xdvh]]></nonce_str>
////<openid><![CDATA[o0phUuBvlowBdXxFBAnzb41ZYxYI]]></openid>
////<out_trade_no><![CDATA[132786550120170614151122]]></out_trade_no>
////<result_code><![CDATA[SUCCESS]]></result_code>
////<return_code><![CDATA[SUCCESS]]></return_code>
////<sign><![CDATA[3ED5801D38714C01DB8E8B27B59A12B0]]></sign>
////<time_end><![CDATA[20170614151136]]></time_end>
////<total_fee>1</total_fee>
////<trade_type><![CDATA[NATIVE]]></trade_type>
////<transaction_id><![CDATA[4005822001201706145689054581]]></transaction_id>
////</xml>

////具体的微信支付回调函数的范例
//func WxpayCallback(w http.ResponseWriter, r *http.Request) {

//	glog.Info("Entry WxpayCallback!!!")
//	//	// body
//	//	body, err := ioutil.ReadAll(r.Body)
//	//	if err != nil {
//	//		glog.Info("读取http body失败，原因!", err)
//	//		http.Error(w.(http.ResponseWriter), http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
//	//		return
//	//	}
//	//	defer r.Body.Close()

//	//	glog.Info("微信支付异步通知，HTTP Body:", string(body))
//	//	var mr WXPayNotifyReq
//	//	err = xml.Unmarshal(body, &mr)
//	//	if err != nil {
//	//		glog.Info("解析HTTP Body格式到xml失败，原因!", err)
//	//		http.Error(w.(http.ResponseWriter), http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
//	//		return
//	//	}

//	//	var reqMap map[string]interface{}
//	//	reqMap = make(map[string]interface{}, 0)

//	//	reqMap["return_code"] = mr.Return_code
//	//	reqMap["return_msg"] = mr.Return_msg
//	//	reqMap["appid"] = mr.Appid
//	//	reqMap["mch_id"] = mr.Mch_id
//	//	reqMap["nonce_str"] = mr.Nonce
//	//	reqMap["result_code"] = mr.Result_code
//	//	reqMap["openid"] = mr.Openid
//	//	reqMap["is_subscribe"] = mr.Is_subscribe
//	//	reqMap["trade_type"] = mr.Trade_type
//	//	reqMap["bank_type"] = mr.Bank_type
//	//	reqMap["total_fee"] = mr.Total_fee
//	//	reqMap["fee_type"] = mr.Fee_type
//	//	reqMap["cash_fee"] = mr.Cash_fee
//	//	reqMap["cash_fee_type"] = mr.Cash_fee_Type
//	//	reqMap["transaction_id"] = mr.Transaction_id
//	//	reqMap["out_trade_no"] = mr.Out_trade_no
//	//	reqMap["attach"] = mr.Attach
//	//	reqMap["time_end"] = mr.Time_end

//	var resp WXPayNotifyResp
//	//进行签名校验
//	//这里就可以更新我们的后台数据库了，其他业务逻辑同理。
//	resp.Return_code = "SUCCESS"
//	resp.Return_msg = "OK"
//	//结果返回，微信要求如果成功需要返回return_code "SUCCESS"
//	bytes, _err := xml.Marshal(resp)
//	strResp := strings.Replace(string(bytes), "WXPayNotifyResp", "xml", -1)
//	if _err != nil {
//		glog.Info("xml编码失败，原因：", _err)
//		http.Error(w.(http.ResponseWriter), http.StatusText(http.StatusBadRequest), http.StatusBadRequest)
//		return
//	}

//	w.(http.ResponseWriter).WriteHeader(http.StatusOK)
//	fmt.Fprint(w.(http.ResponseWriter), strResp)
//}

////首先定义一个UnifyOrderReq用于填入我们要传入的参数。
//type UnifyOrderReq struct {
//	Appid            string `xml:"appid"`
//	Body             string `xml:"body"`
//	Mch_id           string `xml:"mch_id"`
//	Nonce_str        string `xml:"nonce_str"`
//	Notify_url       string `xml:"notify_url"`
//	Trade_type       string `xml:"trade_type"`
//	Spbill_create_ip string `xml:"spbill_create_ip"`
//	Total_fee        int    `xml:"total_fee"`
//	Out_trade_no     string `xml:"out_trade_no"`
//	Sign             string `xml:"sign"`
//}

////微信支付计算签名的函数
//func wxpayCalcSign(mReq map[string]interface{}, key string) (sign string) {
//	glog.Info("微信支付签名计算, API KEY:", key)
//	//STEP 1, 对key进行升序排序.
//	sorted_keys := make([]string, 0)
//	for k, _ := range mReq {
//		sorted_keys = append(sorted_keys, k)
//	}
//	sort.Strings(sorted_keys)
//	//STEP2, 对key=value的键值对用&连接起来，略过空值
//	var signStrings string
//	for _, k := range sorted_keys {
//		glog.Info("k=%v, v=%v\n", k, mReq[k])
//		value := fmt.Sprintf("%v", mReq[k])
//		if value != "" {
//			signStrings = signStrings + k + "=" + value + "&"
//		}
//	}

//	//STEP3, 在键值对的最后加上key=API_KEY
//	if key != "" {
//		signStrings = signStrings + "key=" + key
//	}

//	//STEP4, 进行MD5签名并且将所有字符转为大写.
//	md5Ctx := md5.New()
//	md5Ctx.Write([]byte(signStrings))
//	cipherStr := md5Ctx.Sum(nil)
//	upperSign := strings.ToUpper(hex.EncodeToString(cipherStr))
//	return upperSign
//}

////------------------------------------------------------------------------------

////package main

////import (
////	"crypto/tls"
////	"crypto/x509"
////	"fmt"
////	"io/ioutil"
////	"net/http"
////)

////func main() {
////	pool := x509.NewCertPool()
////	caCertPath := "certs/cert_server/ca.crt"

////	caCrt, err := ioutil.ReadFile(caCertPath)
////	if err != nil {
////		fmt.Println("ReadFile err:", err)
////		return
////	}
////	pool.AppendCertsFromPEM(caCrt)

////	cliCrt, err := tls.LoadX509KeyPair("certs/cert_server/client.crt", "certs/cert_server/client.key")
////	if err != nil {
////		fmt.Println("Loadx509keypair err:", err)
////		return
////	}

////	tr := &http.Transport{
////		TLSClientConfig: &tls.Config{
////			RootCAs:      pool,
////			Certificates: []tls.Certificate{cliCrt},
////		},
////	}
////	client := &http.Client{Transport: tr}
////	resp, err := client.Get("https://server:8081")
////	if err != nil {
////		fmt.Println("Get error:", err)
////		return
////	}
////	defer resp.Body.Close()
////	body, err := ioutil.ReadAll(resp.Body)
////	fmt.Println(string(body))
////}

//代理
//package main

//import (
//	"bufio"
//	"encoding/hex"
//	"encoding/json"
//	"fmt"
//	"io/ioutil"
//	"log"
//	"math/rand"
//	"net"
//	"net/http"
//	"os"
//	"strings"
//	"sync"
//	"time"

//	"github.com/eahydra/socks"
//	"github.com/gamexg/proxyclient"
//	"gopkg.in/alecthomas/kingpin.v2"
//)

//var (
//	proxylist []*AnonymousProxy
//	isOn      bool = false
//	wg        sync.WaitGroup
//	cid       = kingpin.Flag("cid", "room number").Required().Short('c').Int()
//	path      = kingpin.Flag("filepath", "proxy file path").Default("").Short('p').String()
//	retry     = kingpin.Flag("retry", "proxy retry count").Default("5").Short('r').Int()
//	proxies   = kingpin.Flag("proxies", "proxies count").Default("1e9").Short('x').Int()
//	multi     = kingpin.Flag("thread_number", "thread number").Required().Short('t').Int()
//	doc       = `用法:
//-c, --cid：房间号
//-t, --filepath：线程数
//-p, --thread_number：代理文件
//-r, --retry：代理重连次数，默认为5，设置成0为无限
//-x, --proxies：使用代理的最大数目

//使用 -p O 可以获取免费代理。

//注意：如果你使用了代理的话，-t 选项会被识别为一个代理多少线程。

//按 Ctrl+C 退出`
//)

//type Proxy struct {
//	host   string
//	socks4 *socks.Socks4Client
//	socks5 *socks.Socks5Client
//}
//type AnonymousProxy struct {
//	host string
//	// Ip            string
//	// Port          int
//	// Country       string
//	// FullAddres    string
//	// ProxyType     int
//	// Sec           int
//	// AnonymousType int
//}

//type Info struct {
//	Data struct {
//		Status string `json:"_status"`
//	} `json:"data"`
//}

//func AddProxy(tem string) {
//	// tSize := len(tem)
//	// if tSize != 2 {
//	// 	if !(tem[1] == "4" && tSize == 3) && !(tem[1] == "5" && tSize == 4) {
//	// 		fmt.Println(tem)
//	// 		log.Fatal("文件内有错误，请检查！")
//	// 	}
//	// }
//	// Ina := Proxy{Ip: tem[0]}
//	// var user, pass string
//	// if tSize == 4 {
//	// 	user = tem[2]
//	// 	pass = tem[3]
//	// } else if tSize == 3 {
//	// 	user = tem[2]
//	// }
//	// switch tem[1] {
//	// case "4":
//	// 	//Ina.socks4, _ = socks.NewSocks4Client("tcp", Ina.host, user, socks.Direct)
//	// case "5":
//	// 	//Ina.socks5, _ = socks.NewSocks5Client("tcp", Ina.host, user, pass, socks.Direct)
//	// }
//	aProxy := AnonymousProxy{host: tem}
//	proxylist = append(proxylist, &aProxy)
//}

//func GetProxyList() {
//	if *path == "O" {
//		resp, _ := http.Get("http://tvp.daxiangdaili.com/ip/?tid=557816260805608&num=1000000")

//		body, err := ioutil.ReadAll(resp.Body)
//		if err != nil {
//			log.Println(err)
//		}
//		urls := strings.Split(string(body), "\n")
//		for i := range urls {
//			//log.Printf(urls[i])
//			if i < *proxies {
//				AddProxy(urls[i])
//			}
//		}
//		log.Printf("已获取到%d个免费代理……", len(proxylist))
//	} else {
//		file, err := os.Open(*path)
//		if os.IsNotExist(err) {
//			log.Fatal("文件不存在！")
//		}
//		defer file.Close()
//		scanner := bufio.NewScanner(file)
//		for scanner.Scan() {
//			//AddProxy(strings.Split(scanner.Text(), " "))
//		}
//		log.Printf("已读取%d个代理……", len(proxylist))
//	}
//}

//func RandInt(min, max int) int {
//	return rand.Intn(max-min) + min
//}

//func Connector(aproxy *AnonymousProxy, deadCnt int) {
//	if deadCnt != 0 {
//		if deadCnt > *retry {
//			if aproxy != nil {
//				log.Printf("%s无法连通！", aproxy.host)
//			}
//			wg.Done()
//			return
//		}
//		time.Sleep(5 * time.Second)
//	}
//	DialFuc := net.Dial
//	if aproxy != nil && aproxy.host != "" {
//		nv := strings.TrimSpace("http://" + aproxy.host)

//		pc, err := proxyclient.NewProxyClient(nv + "?standardheader=True")
//		if err != nil {
//			fmt.Fprintln(os.Stderr, "can't connect to the proxy:", err)
//			//os.Exit(1)
//		}
//		DialFuc = pc.Dial
//	}
//	conn, err := DialFuc("tcp", "livecmt-1.bilibili.com:788")
//	if err != nil {
//		go Connector(aproxy, deadCnt+1)
//		return
//	}
//	cdata := fmt.Sprintf(`{"roomid":%d,"uid":%d}`, *cid, RandInt(1e5, 3.6e7))
//	handshake := fmt.Sprintf("%08x001000010000000700000001", len(cdata)+16)
//	buf := make([]byte, len(handshake)>>1)
//	hex.Decode(buf, []byte(handshake))
//	_, err = conn.Write(append(buf, []byte(cdata)...))
//	if err != nil {
//		go Connector(aproxy, 0)
//		return
//	}
//	for {
//		buf := make([]byte, 16)
//		hex.Decode(buf, []byte("00000010001000010000000200000001"))
//		_, err := conn.Write(buf)
//		if err != nil {
//			go Connector(aproxy, 0)
//			return
//		}
//		time.Sleep(20 * time.Second)
//	} // should never end
//}

//func GetRoomStatus() bool {
//	var liveInfo Info
//	resp, err := http.Get(fmt.Sprintf("http://live.bilibili.com/live/getInfo?roomid=%d", *cid))
//	if err != nil {
//		time.Sleep(3 * time.Second)
//		return GetRoomStatus()
//	}
//	json.NewDecoder(resp.Body).Decode(&liveInfo)
//	return liveInfo.Data.Status == "on"
//}

//func init() {
//	rand.Seed(time.Now().UnixNano())
//	kingpin.CommandLine.HelpFlag.Short('h')
//	kingpin.UsageTemplate(doc)
//	kingpin.Parse()
//	log.Println("任务开始……")
//	if *path != "" {
//		GetProxyList()
//	}
//	if *retry == 0 {
//		*retry = 1e9
//	}
//	if !GetRoomStatus() {
//		log.Println("正在等待直播间打开……")
//	}
//}

//func Fmain() {
//	size := len(proxylist)
//	isProxied := true
//	if size == 0 {
//		size = 1
//		isProxied = false
//	}
//	log.Println("正在连接……")
//	wg.Add(size * *multi)
//	if isProxied {
//		for j := 0; j < *multi; j++ {
//			for i := 0; i < size; i++ {
//				go Connector(proxylist[i], 0)
//				time.Sleep(100 * time.Microsecond)
//			}
//		}
//	} else {
//		for i := 0; i < *multi; i++ {
//			go Connector(nil, 0)
//		}
//	}
//	log.Println("连接完毕！")
//	wg.Wait()
//	log.Println("所有线程已死亡！退出……")
//	os.Exit(0)
//}

//func main() {
//	for {
//		nowSta := GetRoomStatus()
//		if nowSta != isOn {
//			isOn = nowSta
//			switch nowSta {
//			case true:
//				log.Println("直播间已开启！")
//				go Fmain()
//			case false:
//				log.Println("直播间已关闭！")
//				fmt.Println("最后祝您，____，再见。")
//				return
//			}
//		}
//		time.Sleep(15 * time.Second)
//	}
//}

// 去掉进程
//package main

//import (
//	"syscall"
//)

//func abort(funcname string, err string) {
//	panic(funcname + " failed: " + err)
//}

//func print_version(v uint32) {
//	major := byte(v)
//	minor := uint8(v >> 8)
//	build := uint16(v >> 16)
//	print("windows version ", major, ".", minor, " (Build ", build, ")\n")
//}

//func main() {
//	h, err := syscall.LoadLibrary("kernel32.dll")
//	if err != nil {
//		abort("LoadLibrary", err.Error())
//	}
//	defer syscall.FreeLibrary(h)

//	proc, err := syscall.GetProcAddress(h, "GetVersion")
//	if err != nil {
//		abort("GetProcAddress", err.Error())
//	}
//	r, _, _ := syscall.Syscall(uintptr(proc), 0, 0, 0, 0)
//	print_version(uint32(r))
//}

////windows 7下调试通过
//// 附 一个go站 http://golangwiki.org
////附 error 定义
//type error interface {
//	Error() string
//}

//package main

///*
//#cgo linux LDFLAGS: -lrt

//#include <fcntl.h>
//#include <unistd.h>
//#include <mman.h>

//#define FILE_MODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

//int my_shm_new(char *name) {
//    shm_unlink(name);
//    return shm_open(name, O_RDWR|O_CREAT|O_EXCL, FILE_MODE);
//}
//*/
//import "C"
//import (
//	"fmt"
//	"unsafe"
//)

//const SHM_NAME = "my_shm"
//const SHM_SIZE = 4 * 1000 * 1000 * 1000

//type MyData struct {
//	Col1 int
//	Col2 int
//	Col3 int
//}

//func main() {
//	fd, err := C.my_shm_new(C.CString(SHM_NAME))
//	if err != nil {
//		fmt.Println(err)
//		return
//	}

//	C.ftruncate(fd, SHM_SIZE)

//	ptr, err := C.mmap(nil, SHM_SIZE, C.PROT_READ|C.PROT_WRITE, C.MAP_SHARED, fd, 0)
//	if err != nil {
//		fmt.Println(err)
//		return
//	}
//	C.close(fd)

//	data := (*MyData)(unsafe.Pointer(ptr))

//	data.Col1 = 100
//	data.Col2 = 876
//	data.Col3 = 8021
//}

//package main

///*
//#include <stdio.h>
//#include <stdlib.h>

//void print(char* s) {
//    printf("print: %s\n", s);
//}
//*/
//import "C"
//import (
//	"fmt"
//	"reflect"
//	"runtime"
//	"time"
//	"unsafe"
//)

//type Slice struct {
//	Data []byte
//	data *c_slice_t
//}

//type c_slice_t struct {
//	p unsafe.Pointer
//	n int
//}

//func newSlice(p unsafe.Pointer, n int) *Slice {
//	data := &c_slice_t{p, n}
//	runtime.SetFinalizer(data, func(data *c_slice_t) {
//		println("gc:", data.p)
//		C.free(data.p)
//	})
//	s := &Slice{data: data}
//	h := (*reflect.SliceHeader)((unsafe.Pointer(&s.Data)))
//	h.Cap = n
//	h.Len = n
//	h.Data = uintptr(p)
//	return s
//}

//func testSlice() {
//	msg := "hello world!"
//	p := C.calloc((C.size_t)(len(msg)+1), 1)
//	println("malloc:", p)

//	s := newSlice(p, len(msg)+1)
//	copy(s.Data, []byte(msg))

//	fmt.Printf("fmt.Printf: %s\n", string(s.Data))
//	C.print((*C.char)(p))
//}

//func main() {
//	testSlice()

//	runtime.GC()
//	runtime.Gosched()
//	time.Sleep(1e9)
//}
