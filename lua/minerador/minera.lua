-- LUA Minerador Duino-Coin v1.0
-- Desenvolvido por Albério Lima
-- 2022
--

-- Inicia pacotes necessários
local socket = require("socket")
local http = require("http.request") 
--local http = require("socket.http")
local json = require("cjson")
local sha1 = require("sha1")

-- Variávels DUINO-COIN
local duino_coin_user = "ridimuim"
local duino_coin_diff = "ESP8266H"
local duino_coin_banner = "Official ESP8266 Miner"
local duino_coin_version = "3.0"
local duino_coin_ducoid = "DUCOID0102030405060708"
local duino_coin_rig_id = "None"
local duino_coin_miner_key = "None"

-- Inicia variáveis publicas 
host, port = "51.158.113.59", 14166 -- Endereço do servidor, será modificado por get_pool()
share_count = 0; -- Contador de shares goods
pck_request_jog = "JOB,"..duino_coin_user..","..duino_coin_diff..","..duino_coin_miner_key.."\n" -- pacote de solicitação de trabalho (job)

--Função que divide os dados, criando uma tabela (array)
local function split(str, sep)
   local result = {}
   local regex = ("([^%s]+)"):format(sep)
   for each in str:gmatch(regex) do
      table.insert(result, each)
   end
   return result
end

-- Função que pega o endereço do servidor (pool) 
local function get_pool()
   local headers, stream = assert(http.new_from_uri("https://server.duinocoin.com/getPool"):go())
   local body = assert(stream:get_body_as_string())
   -- body = http.request("https://server.duinocoin.com/getPool")
   j = json.decode(body);
   host = j["ip"]
   port = j["port"]
end

-- Inicia conexão TCP
tcp = assert(socket.tcp())
tcp:settimeout(10)

print("Minerador duinocoin")
print("lua:",_VERSION)
print("sha1:", sha1.version)
while true do
  get_pool()
  print("")
  print("Conectando ao servidor")
  print("IP:", host)
  print("porta:", port)
  tcp_connected = tcp:connect(host, port)
  if tcp_connected == 1 then
    print("conectado")
    
    -- recebe versão do servidor
    local s, err = tcp:receive()
    if s ~= nil then
      print("Versão:",s)
    end
    
    while true do
      
      -- solicita job
      tcp:send(pck_request_jog)
      local s, status, partial = tcp:receive()
      -- print(s or partial)
      if status == "closed" then 
        break 
      end
      
      -- desempacota dados
      dados = split(s,",")
      block = dados[1]
      esperado = dados[2]
      diff = tonumber(dados[3])
      diff = (diff * 100 ) + 1
      
      -- executa job
      local start_time = os.clock()
      for i = 0, diff, 1 do
        local message = block..tostring(i)
        local hash_as_hex = sha1.sha1(message)
        if hash_as_hex == esperado then 
          -- calcula hashrate e tempo decorrido
          tempo_decorrido = os.clock() - start_time
          hashrate = math.floor(i / tempo_decorrido)
          if hashrate < 220 then
            hashrate = 220
          end
          
          -- empacota e envia mensagem de resposta
          local mens = tostring(i)
          mens = mens..","
          mens = mens..tostring(hashrate)
          mens = mens..","..duino_coin_banner.." "..duino_coin_version..","..duino_coin_rig_id..","..duino_coin_ducoid.."\n"        
          tcp:send(mens)
          
          -- trata retorno 
          local s,err = tcp:receive()
          if s == "GOOD" then
            share_count = share_count + 1
          end
          
          -- debug 
          local mens_debug = s
          mens_debug = mens_debug.." share #"..tostring(share_count)
          mens_debug = mens_debug.." ("..tostring(i)..") hashate: "
          mens_debug = mens_debug..string.format("%.3f", hashrate/1000).." kH/s ("
          mens_debug = mens_debug..string.format("%.3f", tempo_decorrido).."s)"          
          print(mens_debug)
          break
        end
      end
    end    
  end  
  tcp:close()
end
