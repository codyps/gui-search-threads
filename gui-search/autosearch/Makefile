

.PHONY: all clean indexer search indexer-c search-c

all: indexer search 

search:
	$(MAKE) -C $@

indexer:
	$(MAKE) -C $@

indexer-c:
	$(MAKE) -C $(@:-c=) clean

search-c:
	$(MAKE) -C $(@:-c=) clean

clean: indexer-c search-c 
