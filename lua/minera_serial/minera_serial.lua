-- LUA Minerador Duino-Coin v1.0
-- Para escravo conectado a porta serial 
-- Desenvolvido por Albério Lima
-- 2022
--
--opkg install lua-rs232
--sudo luarocks install luars232 (no PC)

-- Inicia pacotes necessários
local socket = require("socket")
local http = require("http.request")
--local http = require("socket.http")
local json = require("cjson")
local rs232 = require("luars232")
local out = io.stderr

-- Variávels DUINO-COIN
local duino_coin_user = "ridimuim"
local duino_coin_diff = "AVR"
local duino_coin_banner = "Official AVR Miner"
local duino_coin_version = "3.0"
local duino_coin_rig_id = "None"
local duino_coin_miner_key = "None"

-- Inicia variáveis publicas 
host, port = "51.158.113.59", 14166
share_count = 0;
pck_request_jog = "JOB,"..duino_coin_user..","..duino_coin_diff..","..duino_coin_miner_key.."\n"
port_name = "/dev/ttyUSB0" -- port_name = "COM1"

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

-- abre porta serial
local e, p = rs232.open(port_name)
if e ~= rs232.RS232_ERR_NOERROR then
	-- handle error
	out:write(string.format("can't open serial port '%s', error: '%s'\n",
			port_name, rs232.error_tostring(e)))
	return
end

-- configura porta serial
assert(p:set_baud_rate(rs232.RS232_BAUD_115200) == rs232.RS232_ERR_NOERROR)
assert(p:set_data_bits(rs232.RS232_DATA_8) == rs232.RS232_ERR_NOERROR)
assert(p:set_parity(rs232.RS232_PARITY_NONE) == rs232.RS232_ERR_NOERROR)
assert(p:set_stop_bits(rs232.RS232_STOP_1) == rs232.RS232_ERR_NOERROR)
assert(p:set_flow_control(rs232.RS232_FLOW_OFF)  == rs232.RS232_ERR_NOERROR)
out:write(string.format("OK, port open with values '%s'\n", tostring(p)))

-- Inicia conexão TCP
tcp = assert(socket.tcp())
tcp:settimeout(10)

print("Minerador duinocoin")
print("Lua:",_VERSION)
while true do
  get_pool()
  print("")
  print("Conectando ao servidor")
  print("IP:", host)
  print("porta:", port)
  tcp_connected = tcp:connect(host, port)
  if tcp_connected == 1 then
    print("conectado")
    
    --recebe versão do servidor
    local s, err = tcp:receive()
    if s ~= nil then
      print("Versão:",s)
    end
    
    print("Iniciando mineração")
    while true do
      
      --solicita job
      tcp:send(pck_request_jog)
      local s, status, partial = tcp:receive()
      --print(s or partial)
      if status == "closed" then 
        break 
      end
      
      --envia pacote para minerador
      s = s..",\n"
      err, len_written = p:write(s)
      assert(e == rs232.RS232_ERR_NOERROR)
      
      --aguarda resposta do minerador
      s = ""
      data_read = "*"
      while data_read ~= '\n' do
        err, data_read, size = p:read(1, 5000)
        if size > 0 then
          s = s..data_read
        end
      end      
      
      --desempacota dados recebidos do minerador
      dados = split(s, ",")
      tempo_dec_s = tonumber(dados[2],2)/1000000
      duco_result = tonumber(dados[1],2)
      hashrate = math.floor( duco_result / tempo_dec_s)
      
      -- empacota e envia mensagem de resposta      
      local mens = tostring(tonumber(dados[1],2))
      mens = mens..","
      mens = mens..tostring(hashrate)
      mens = mens..","..duino_coin_banner.." "..duino_coin_version..","..duino_coin_rig_id..","
      mens = mens..dados[3]
      mens = mens.."\n"
      tcp:send(mens)
         
      --trata retorno do servidor 
      local s,err = tcp:receive()
      if s == "GOOD" then
        share_count = share_count + 1
      end
      
      -- debug 
      local mens_debug = s
      mens_debug = mens_debug.." share #"..tostring(share_count)
      mens_debug = mens_debug.." ("..tostring(duco_result)..") hashate: "
      mens_debug = mens_debug..string.format("%.3f", hashrate/1000).." kH/s ("
      mens_debug = mens_debug..string.format("%.3f", tempo_dec_s).."s)"          
      print(mens_debug)
    end    
  end  
  tcp:close()
end
assert(p:close() == rs232.RS232_ERR_NOERROR)
