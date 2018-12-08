package main

import (
	"encoding/xml"
	"fmt"
	"io/ioutil"
	//	"os"
	//    "path/filepath"
	//    "strings"
)

type Result struct {
	Person []Person `xml:"config"`
}
type Person struct {
	Name string `xml:"version,attr"`
}

var GGGMap_Ver map[string]string

func mainliuliu(StrGame string, strPath string) {

	//strPath := getCurrentPath()
	content, err := ioutil.ReadFile(strPath + StrGame + "//" + StrGame + "_Data" + "//StreamingAssets//version.xml")
	if err != nil {
		fmt.Println(err)
		return
	}
	var result Result
	err = xml.Unmarshal(content, &result)
	if err != nil {
		fmt.Println(err)
		return
	}
	for _, o := range result.Person {
		fmt.Println("版本：--", o.Name)
		GGGMap_Ver[StrGame] = o.Name
	}
	return
}

//获取指定目录下的所有文件和目录
func ListDir(dirPth string) {
	//fmt.Println(dirPth)
	dir, err := ioutil.ReadDir(dirPth)
	if err != nil {
		return
	}
	//PthSep := string(os.PathSeparator)
	//    suffix = strings.ToUpper(suffix) //忽略后缀匹配的大小写
	for _, fi := range dir {

		if fi.IsDir() { // 忽略目录
			//files1 = append(files1, dirPth+PthSep+fi.Name())
			//ListDir(dirPth + PthSep + fi.Name())
			fmt.Println(fi.Name())
			mainliuliu(fi.Name(), dirPth)
		}
	}
	return
}
