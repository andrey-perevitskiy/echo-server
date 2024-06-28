.PHONY: all
all: server client

.PHONY: server
server:
	$(MAKE) -C server

.PHONY: client
client:
	$(MAKE) -C client

.PHONY: clean
clean:
	rm -rf bin common/obj

.PHONY: clean-all
clean-all: clean
	$(MAKE) -C client clean
	$(MAKE) -C server clean
