package rest

import (
	"encoding/base64"
	"fmt"
	"strconv"
	"strings"
)

func AtoU(str string) (string, string) {
	sUnicodev := strings.Split(str, "\\u")
	if len(sUnicodev) < 2 {
		return str, ""
	}
	var context string
	for i, v := range sUnicodev {
		if len(v) < 1 || i == 0 {
			context += v
			continue
		}
		temp, err := strconv.ParseInt(Substr(v, 0, 4), 16, 32)
		if err != nil {
			panic(err)
		}
		Surplus := Substr(v, 4, len(v)-4)
		context += fmt.Sprintf("%c", temp)
		context += Surplus
	}
	//fmt.Println(context)
	return context, ""
}
func Substr(str string, start, length int) string {
	rs := []rune(str)
	rl := len(rs)
	end := 0

	if start < 0 {
		start = rl - 1 + start
	}
	end = start + length

	if start > end {
		start, end = end, start
	}

	if start < 0 {
		start = 0
	}
	if start > rl {
		start = rl
	}
	if end < 0 {
		end = 0
	}
	if end > rl {
		end = rl
	}
	return string(rs[start:end])
}
func Safe_Base64Decode(sstr string) (string, error) {
	str := sstr
	str = strings.Replace(str, "%2F", "/", -1)
	str = strings.Replace(str, "%3D", "=", -1)
	str = strings.Replace(str, "%2B", "+", -1)
	str = strings.Replace(str, "%2f", "/", -1)
	str = strings.Replace(str, "%3d", "=", -1)
	str = strings.Replace(str, "%2b", "+", -1)
	decodeBytes, err := base64.StdEncoding.DecodeString(str)
	if err != nil {
		fmt.Println(err)
	}
	return string(decodeBytes), err
}
