PROTOC_DIR=/usr/
OBJ_DIR = object
INC_DIR = include
LIB_DIR = lib
LDFLAGS = ${PROTOC_DIR}lib/x86_64-linux-gnu/libprotobuf-c.a -lpthread -lzookeeper_mt
CFLAGS = -Wall -O2 -I $(INC_DIR) -g -lpthread -lzookeeper_mt
CC = gcc
LD = ld
OBJETOS_SERVER = tree_server.o tree_skel.o network_server.o message.o sdmessage.pb-c.o tree.o entry.o data.o
OBJETOS_CLIENT = sdmessage.pb-c.o tree_client.o message.o tree.o
OBJETOS_LIB = client_stub.o network_client.o data.o entry.o
TARGETS = sdmessage.pb-c client-lib.o tree-client tree-server
data.o = data.h
entry.o = entry.h data.h
tree.o = tree.h tree-private.h data.h entry.h
client_stub.o = client_stub.h network_client.h client_stub-private.h sdmessage.pb-c.h
message.o = message-private.h
network_client.o = network_client.h client_stub-private.h sdmessage.pb-c.h message-private.h
tree_client.o = client_stub.h data.h entry.h tree.h tree_client-private.h
tree_server.o = tree_server-private.h tree_skel.h network_server.h tree_skel-private.h
network_server.o = network_server.h message-private.h sdmessage.pb-c.h network_server-private.h
tree_skel.o = tree_skel.h tree.h sdmessage.pb-c.h network_server.h tree_skel-private.h
sdmessage.pb-c.o = sdmessage.pb-c.h
client-lib.o = client_stub.h network_client.h data.h entry.h

ALL:$(TARGETS)

client-lib.o: $(OBJETOS_LIB)
	$(LD) -r $(addprefix $(OBJ_DIR)/,$(OBJETOS_LIB)) -o $(LIB_DIR)/client-lib.o  

tree-server: $(OBJETOS_SERVER)
	$(CC) $(addprefix $(OBJ_DIR)/,$(OBJETOS_SERVER)) -o binary/tree-server ${LDFLAGS} 

tree-client: $(OBJETOS_CLIENT)
	$(CC) $(addprefix $(OBJ_DIR)/,$(OBJETOS_CLIENT)) lib/client-lib.o -o binary/tree-client ${LDFLAGS} 
	
sdmessage.pb-c:sdmessage.proto
	${PROTOC_DIR}bin/protoc-c sdmessage.proto --c_out=.
	mv sdmessage.pb-c.c source
	mv sdmessage.pb-c.h include

sdmessage.pb-c.o: source/sdmessage.pb-c.c $($@)
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/$@ -c $<

%.o: source/%.c $($@)
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/$@ -c $<	

clean:
	rm -f $(OBJ_DIR)/*.o binary/tree-server binary/tree-client
	rm -f $(LIB_DIR)/client-lib.o
	rm -f binary/tree-server binary/tree-client



