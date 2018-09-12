package main

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"redis"
	"reflect"
	"regexp"
	"strings"
	"text/template"
)

var filter1, filter2, filter3 *regexp.Regexp
var LF []byte = []byte("\n")

func init() {
	var e error
	filter1, e = regexp.Compile("\n( |\t)*\n")
	if e != nil {
		panic(e)
	}
	filter2, e = regexp.Compile("\n{3,}\n")
	if e != nil {
		//	filter2, e = regexp.Compile("\t*\n"); if e != nil {
		panic(e)
	}
	filter3, e = regexp.Compile("\n{2,}\n")
	if e != nil {
		panic(e)
	}
}
func main() {
	fmt.Printf("Salaam!\n")

	defer func() {
		if e := recover(); e != nil {
			fmt.Printf("Error - %x", e)
			panic(1)
		}
	}()

	for _, ctype := range getSubjects() {
		fmt.Printf("\tctype: %s\n", ctype)

		t := loadMethodTestTemplate(ctype)

		path := verifyOrCreateDir(ctype)
		for i := 0; i < ctype.NumMethod(); i++ {
			m := ctype.Method(i)
			var buf bytes.Buffer
			renderFeatureTest(&buf, ctype, m, t)
			data := cleanup(buf.Bytes())
			writeTestFile(path, data, ctype, m)
		}
		gofmt(path)
	}
}

func gofmt(path string) (out []byte) {

	gofmt := exec.Command("go", "fmt", path)
	var fmtout, fmterr bytes.Buffer
	gofmt.Stdout = &fmtout
	gofmt.Stderr = &fmterr
	if e := gofmt.Run(); e != nil {
		fmt.Printf("gofmt --error out:\n%s\n", fmterr.Bytes())
		panic(e)
	}
	return fmtout.Bytes()
}

func verifyOrCreateDir(ctype reflect.Type) (dir string) {
	path := filepath.Join(testdir, ctype.Name())
	if e := os.MkdirAll(path, 0764); e != nil {
		panic(fmt.Errorf("verifyOrCreateDir - %s", e))
	}
	return path
}
func cleanup(data []byte) []byte {
	data = filter1.ReplaceAll(data, LF)
	data = filter2.ReplaceAll(data, []byte("\n\n"))
	data = filter3.ReplaceAll(data, []byte("\n"))
	return data
}

// REVU - conf this
var testdir = ".."

func loadMethodTestTemplate(subject reflect.Type) *template.Template {
	//	dir := subject.Name()
	tfn := fmt.Sprintf("%s_mtest.tmpl", subject.Name())
	fn := filepath.Join(tfn)
	t, e := template.New(tfn).Funcs(funcmap()).ParseFiles(fn)
	if e != nil {
		fmt.Printf("Error creating template for %s - %s", subject, e)
		panic(e)
	}
	return t
}

func writeTestFile(path string, data []byte, subject reflect.Type, method reflect.Method) {
	fname := filepath.Join(path, fmt.Sprintf("%s_test.go", method.Name))
	if e := ioutil.WriteFile(fname, data, 0644); e != nil {
		panic(fmt.Errorf("error on WriteFile (%s) - %s", fname, e))
	}
}

type mtest struct {
	Type    reflect.Type
	Subject string
	Method  string
	InArgs  []string
	InCnt   int
	//	OutArgs []string
	OutArgs []outarg
	OutCnt  int
	Spec    *redis.MethodSpec
}
type outarg struct {
	Type string

	IsFuture      bool
	FutureMethod  string
	FutureOutArgs []string
	FutureOutCnt  int
}

func getMethodInArgs(m reflect.Method) []string {
	mt := m.Type
	args := make([]string, mt.NumIn())
	for i := 0; i < mt.NumIn(); i++ {
		args[i] = fmt.Sprintf("%s", mt.In(i))
	}
	return args
}

func getMethodOutArgs(m reflect.Method) []string {
	mt := m.Type
	args := make([]string, mt.NumOut())
	for i := 0; i < mt.NumOut(); i++ {
		otype := mt.Out(i)
		args[i] = fmt.Sprintf("%s", otype)
	}
	return args
}

func getMethodOutArgsWIP(m reflect.Method) []outarg {
	mt := m.Type
	args := make([]outarg, mt.NumOut())
	for i := 0; i < mt.NumOut(); i++ {
		arg := outarg{}
		otype := mt.Out(i)
		arg.Type = fmt.Sprintf("%s", otype)

		// future results only
		if strings.HasPrefix(otype.Name(), "Future") {
			arg.IsFuture = true

			fvalGet, ok := otype.MethodByName("Get")
			if !ok {
				panic(fmt.Errorf("Future type %s does not have a Get method!", otype))
			}

			arg.FutureMethod = "Get"
			arg.FutureOutArgs = getMethodOutArgs(fvalGet)
			arg.FutureOutCnt = len(arg.FutureOutArgs)

			// TODO TryGets ...
			//			fmt.Printf("\t%s\n", getMethodOutArgs(fvalGet))
			//			fvalTryGet, ok := otype.MethodByName("TryGet"); if !ok {
			//				panic(fmt.Errorf("Future type %s does not have a TryGet method!", otype))
			//			}
			//			fmt.Printf("\t%s\n", fvalTryGet)
		}
		args[i] = arg
	}
	return args
}

func renderFeatureTest(w io.Writer, ctype reflect.Type, m reflect.Method, t *template.Template) {
	ins := getMethodInArgs(m)
	outs := getMethodOutArgsWIP(m)
	data := &mtest{
		Type:    ctype,
		Subject: ctype.Name(),
		Method:  m.Name,
		InArgs:  ins,
		InCnt:   len(ins),
		OutArgs: outs,
		OutCnt:  len(outs),
		Spec:    redis.GetMethodSpec(ctype.Name(), m.Name),
	}
	t.Execute(w, data)
}

func getSubjects() []reflect.Type {
	return []reflect.Type{
		reflect.TypeOf((*redis.Client)(nil)).Elem(),
		reflect.TypeOf((*redis.AsyncClient)(nil)).Elem(),
	}
}

func funcmap() map[string]interface{} {
	return template.FuncMap{
		"zvTest":       zvTest,
		"comma":        comma,
		"isRedisError": isRedisError,
		"isFuture":     isFuture,
		"isQuit":       isQuit,
	}
}

func zvTest(tname string) string {
	switch tname {
	case "int64":
		return " != 0"
	case "float64":
		return " != float64(0)"
	case "bool":
		return " != false"
	case "string":
		return " != \"\""

	case "redis.KeyType":
		return " != redis.KeyType(0)"
	}
	return " != nil"
}

func comma(idx, cnt int) string {
	if idx < cnt-1 {
		return ", "
	}
	return ""
}

func isRedisError(v string) string {
	if v == "redis.Error" {
		return "true"
	}
	return ""
}

func isFuture(v string) string {
	if strings.HasPrefix(v, "redis.Future") {
		return "true"
	}
	return ""
}

func isQuit(v string) string {
	if v == "Quit" {
		return "true"
	}
	return ""
}
