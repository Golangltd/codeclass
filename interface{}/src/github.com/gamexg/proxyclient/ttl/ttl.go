package main

/*

ttl 反劫持功能

目前的策略：

发现 http 连接将要建立时立刻：
* 发送一些错误数据
* 重置连接

通过设置 ttl 值使得错误数据及重置连接命令不会达到真正的目标网站，仅仅只传递到可能存在的劫持服务器。

*/

import (
	"fmt"
	"github.com/google/gopacket"
	"github.com/google/gopacket/layers"
	"github.com/google/gopacket/pcap"
	"net"
	"os"
	"sync"
)

func main() {
	devs, err := pcap.FindAllDevs()
	if err != nil {
		panic(err)
	}

	wg := sync.WaitGroup{}
	for _, dev := range devs {
		wg.Add(1)
		dev := dev
		fmt.Println("开始混淆：")
		fmt.Println(dev.Name)
		fmt.Println(dev.Addresses)
		fmt.Println(dev.Description)
		fmt.Println("")
		fmt.Println("")
		go func() {
			defer wg.Done()
			capturePacket(dev.Name)
		}()
	}
	wg.Wait()
}

func capturePacket(deviceName string) {
	if handle, err := pcap.OpenLive(deviceName, 1600, false, pcap.BlockForever); err != nil {
		fmt.Println("监听失败，请确认安装了 pacp 库。详细信息：", err)
		os.Exit(-1)
		// 捕获所有 源端口是 80 的 SYN、ACK 包
	} else if err := handle.SetBPFFilter("tcp and src port 80 and tcp[13] == 0x12"); err != nil {
		//	} else if err := handle.SetBPFFilter("tcp and ((dst port 80 and tcp[13] == 0x02)or (src port 80 and tcp[13] == 0x12))"); err != nil {  // optional
		panic(err)
	} else {
		defer handle.Close()
		packetSource := gopacket.NewPacketSource(handle, handle.LinkType())
		for packet := range packetSource.Packets() {
			handlePacket(handle, packet)
		}
	}
}

func handlePacket(handle *pcap.Handle, packet gopacket.Packet) {
	ethLayer := packet.LinkLayer()
	if ethLayer == nil {
		return
	}
	eth, ok := ethLayer.(*layers.Ethernet)
	if !ok {
		return
	}

	ipLayer := packet.NetworkLayer()
	if ipLayer == nil {
		return
	}

	ip, ok := ipLayer.(*layers.IPv4)
	if !ok {
		return
	}

	tcpLayer := packet.Layer(layers.LayerTypeTCP)
	if tcpLayer == nil {
		return
	}
	tcp, ok := tcpLayer.(*layers.TCP)
	if !ok {
		return
	}

	// 服务器回应 SYN 请求
	if tcp.SYN == true && tcp.ACK == true {
		ttl := uint8(0)

		switch {
		case ip.TTL > 128:
			//Solaris/AIX
			ttl = 254 - ip.TTL
		case ip.TTL > 64:
			// windows
			ttl = 128 - ip.TTL
		default:
			// linux
			ttl = 64 - ip.TTL
		}

		switch {
		case ttl > 10:
			ttl -= 4
		case ttl > 5:
			ttl -= 2
		case ttl > 2:
			ttl -= 1
		default:
			return
		}

		ack := tcp.Seq + uint32(len(tcp.Payload)) + 1
		data := []byte{1, 2, 3, 4, 5, 6, 7, 8, 9}
		seq := tcp.Ack
		sendPacket(handle, eth.DstMAC, eth.SrcMAC, ip.DstIP, ip.SrcIP, tcp.DstPort, tcp.SrcPort, ip.Id+123, ttl, seq, ack, 258, data)
		seq += 2048
		sendPacket(handle, eth.DstMAC, eth.SrcMAC, ip.DstIP, ip.SrcIP, tcp.DstPort, tcp.SrcPort, ip.Id+123, ttl, seq, ack, 258, nil)
		//go fmt.Printf("伪重置 %v:%v 的 tcp 连接。\r\n", ip.SrcIP, tcp.SrcPort)
	}
}

func sendPacket(handle *pcap.Handle, sMac, dMac net.HardwareAddr, sIp, dIp net.IP, sPort, dPort layers.TCPPort, IpId uint16, IpTtl uint8, TcpSeq, ack uint32, WindowsSize uint16, data []byte) error {
	eth := layers.Ethernet{
		SrcMAC:       sMac,
		DstMAC:       dMac,
		EthernetType: layers.EthernetTypeIPv4,
	}
	ip4 := layers.IPv4{
		SrcIP:    sIp,
		DstIP:    dIp,
		Id:       IpId,
		Flags:    layers.IPv4DontFragment,
		Version:  4,
		TTL:      IpTtl,
		Protocol: layers.IPProtocolTCP,
	}
	tcp := layers.TCP{
		SrcPort: sPort,
		DstPort: dPort,
		Seq:     TcpSeq,
		ACK:     true,
		Ack:     ack,
		Window:  WindowsSize,
		PSH:     true, // 立刻处理
	}

	if len(data) == 0 {
		tcp.RST = true
	}

	tcp.SetNetworkLayerForChecksum(&ip4)

	buf := gopacket.NewSerializeBuffer()
	opts := gopacket.SerializeOptions{
		FixLengths:       true,
		ComputeChecksums: true,
	}

	payload := gopacket.Payload(data)

	if err := gopacket.SerializeLayers(buf, opts, &eth, &ip4, &tcp, payload); err != nil {
		return err
	}

	return handle.WritePacketData(buf.Bytes())
}
