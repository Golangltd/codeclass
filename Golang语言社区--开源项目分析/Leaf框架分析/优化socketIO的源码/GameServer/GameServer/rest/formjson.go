package rest

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"mime"
	"net/http"
	"strings"
)

//Middleware for converting posted form data into json
type FormJsonMiddleware struct{}

func (mw *FormJsonMiddleware) MiddlewareFunc(handler HandlerFunc) HandlerFunc {

	return func(w ResponseWriter, r *Request) {

		mediatype, params, _ := mime.ParseMediaType(r.Header.Get("Content-Type"))
		if mediatype == "application/x-www-form-urlencoded" {
			// get body
			buf, _ := ioutil.ReadAll(r.Body)
			charset, ok := params["charset"]
			if ok && strings.ToUpper(charset) == "UTF-8" {

			}
			// map body form data
			jsonMap := map[string]string{}
			sections := strings.Split(string(buf), "&")
			for _, sectionValue := range sections {
				sectionParts := strings.Split(sectionValue, "=")
				if len(sectionParts) == 2 {
					jsonMap[sectionParts[0]] = sectionParts[1]
				} else {
					//error converting, skip to handler
					mw.conversionError(w)
					return
				}
			}

			//marshal json
			jsonString, err := json.Marshal(jsonMap)
			if err != nil {
				//error marshalling, skip to handler
				mw.conversionError(w)
				return
			}

			//write new body
			fmt.Println(string(jsonString))
			r.Body = ioutil.NopCloser(bytes.NewReader([]byte(string(jsonString))))

			//convert content-type header
			r.Header.Set("Content-Type", "application/json")
		}
		// call the wrapped handle
		handler(w, r)
	}
}

func (mw *FormJsonMiddleware) conversionError(w ResponseWriter) {
	Error(w, "Error Converting Form Data", http.StatusInternalServerError)
}
