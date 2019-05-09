package main

import (
	"bufio"
	"fmt"
	"os"
	"strconv"
	"strings"
)

/*
  Go语言控制台抽奖
  1. 抽奖人，读取配置文件（CSV）
  2. 随机方法，随机数
  3. 中奖人信息写入文件
*/

func init() {

}

func main() {
	reader := bufio.NewReader(os.Stdin)
	fmt.Println("--= www.ByteEdu.Com =--")
	fmt.Println("--------活动抽奖--------")
	fmt.Println("输入规则：")
	fmt.Println("A|1:  A表示一等奖，1表示数量")
	fmt.Println("B|2:  B表示二等奖，2表示数量")
	fmt.Println("C|3:  C表示三等奖，3表示数量")
	fmt.Println("注:  数量可以变化，例如A|12等")

	tmpmap := make(map[int]int)
	for {
		fmt.Print("-> ")
		text, _ := reader.ReadString('\n')
		text = strings.Replace(text, "\r\n", "", -1)
		// 截取字符
		data := strings.Split(text, "|")
		if len(data) != 2 {
			fmt.Print("输入错误！")
			continue
		}
		// 获取随机数
		count, errrr := strconv.Atoi(data[1])
		if errrr != nil {
			fmt.Println(errrr)
			return
		}
		if count >= len(G_ByteEdu) {
			fmt.Println("全体中奖！,本轮抽奖结束！")
			return
		}
		var icount []int
		for iBINGE := 0; iBINGE < int(count); iBINGE++ {
			ii := RandInterval_LollipopGo(0, int32(len(G_ByteEdu)))
			icount = append(icount, int(ii))
		}
		if len(icount) == 0 {
			fmt.Println("抽奖名单为空，请检查抽奖名单！，抽奖结束！")
			return
		}
		switch data[0] {
		case "A":
			{
				fmt.Println("抽取一等奖!")
				for i, v := range icount {
					fmt.Println(i, v)
					_, ok := tmpmap[v]
					if !ok {
						msg := "恭喜：" + G_ByteEdu[v].Name + "，获得一等奖!"
						fmt.Println(msg)
						tmpmap[v] = v
						WriteFileData(msg)
					} else {
						fmt.Println("很遗憾，无人中奖！")
					}
				}
			}
		case "B":
			{
				fmt.Println("抽取二等奖!")
				for i, v := range icount {
					fmt.Println(i, v)
					_, ok := tmpmap[v]
					if !ok {
						fmt.Println("恭喜：" + G_ByteEdu[v].Name + "，获得二等奖!")
						tmpmap[v] = v
					} else {
						fmt.Println("很遗憾，无人中奖！")
					}
				}
			}
		case "C":
			{
				fmt.Println("抽取三等奖!")
				for i, v := range icount {
					fmt.Println(i, v)
					_, ok := tmpmap[v]
					if !ok {
						fmt.Println("恭喜：" + G_ByteEdu[v].Name + "，获得三等奖!")
						tmpmap[v] = v
					} else {
						fmt.Println("很遗憾，无人中奖！")
					}
				}
			}
		default:
			fmt.Println("没有中奖！")
		}
	}
}
