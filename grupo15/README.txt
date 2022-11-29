Grupo n: 15
Afonso Santos - FC56368
Alexandre Figueiredo - FC57099
Raquel Domingos - FC56378

1-Em entry.c, na funcao entry_compare foi assumido que nao eh necessario considerar o caso de algum
dos parametros ser NULL visto que nao esta dito no enunciado o que fazer nessa situacao.

2-Em sdmessage.proto nao foram definidos campos opcionais uma vez que definindo-os, ao compilar surgia
a mensagem de erro "sdmessage.proto: This file contains proto3 optional fields, but --experimental_allow_proto3_optional 
was not set.". Ao compilar com a flag indicada surgia a mensagem de erro "sdmessage.proto: is a proto3 file that contains 
optional fields, but code generator --c_out hasn't been updated to support optional fields in proto3. Please ask the owner 
of this code generator to support proto3 optional."

3-Em sdmessage.proto decidimos colocar repeated bytes uma vez que satisfaz as necessidades para o getvalues e todas
as outras funcoes que envolvem chaves. Por essa razao e pelo facto do processo ser semelhante caso tenhamos criado
um campo especifico para as chaves, nao consideramos haver necessidade de criar um campo repeated string. Para os casos 
onde eh necessario enviar numeros, decidimos criar um campo int32, visto que seria mais legivel do que reutilizar
o campo repeated bytes.