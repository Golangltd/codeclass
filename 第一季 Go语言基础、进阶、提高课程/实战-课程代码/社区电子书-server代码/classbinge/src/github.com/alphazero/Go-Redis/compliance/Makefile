include $(GOROOT)/src/Make.inc

%:
#	echo making $@
	$(GC) -o $@.$(O) $@.go
	$(LD) -o $@ $@.$(O)

