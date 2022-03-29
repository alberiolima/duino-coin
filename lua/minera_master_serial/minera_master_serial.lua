-- LUA Minerador Duino-Coin v1.0
-- Para escravos conectado via I2C a uma ponte serial-i2c
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
maximo_clientes = 1
host, port = "51.158.113.59", 14166
share_count = 0;
pck_request_jog = "JOB,"..duino_coin_user..","..duino_coin_diff..","..duino_coin_miner_key.."\n"
port_name = "/dev/ttyACM0" -- port_name = "COM1"

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
--assert(p:set_baud_rate(rs232.RS232_BAUD_115200) == rs232.RS232_ERR_NOERROR)
assert(p:set_baud_rate(rs232.RS232_BAUD_57600) == rs232.RS232_ERR_NOERROR)
assert(p:set_data_bits(rs232.RS232_DATA_8) == rs232.RS232_ERR_NOERROR)
assert(p:set_parity(rs232.RS232_PARITY_NONE) == rs232.RS232_ERR_NOERROR)
assert(p:set_stop_bits(rs232.RS232_STOP_1) == rs232.RS232_ERR_NOERROR)
assert(p:set_flow_control(rs232.RS232_FLOW_OFF)  == rs232.RS232_ERR_NOERROR)
out:write(string.format("OK, port open with values '%s'\n", tostring(p)))


local conexoes = {}
local status_conexoes = {}
local operacao = {}
local n_cliente = 1;
local n_clientes_ativos = 1 -- maximo_clientes
local serial_buffer = ""
print("Minerador duinocoin")
print("Lua:",_VERSION)
--delay
local t_final = os.clock() + 3
while os.clock() < t_final do end
while true do
  if status_conexoes[n_cliente] == nil then
    operacao[n_cliente] = 0 -- sem conexão
    get_pool()
    print("")
    print("Conectando ao servidor")
    print("IP:", host)
    print("porta:", port)
    conexoes[n_cliente] = socket.tcp()
    conexoes[n_cliente]:settimeout(2)
    status_conexoes[n_cliente] = conexoes[n_cliente]:connect(host, port)
    print("conectado:",status_conexoes[n_cliente])
    if status_conexoes[n_cliente] ~= nil then
      operacao[n_cliente] = 1 -- solicitar job 
      --recebe versão do servidor
      local s, err = conexoes[n_cliente]:receive(3)
      if s ~= nil then
        print("Versão:",s)
      end
    end  
  end
  
  --solicita trabalho (JOB)
  if operacao[n_cliente] == 1 then
    conexoes[n_cliente]:send(pck_request_jog)
    local job, status, partial = conexoes[n_cliente]:receive()
    --print(job or partial)
    if status == "closed" then 
      status_conexoes[n_cliente] = nil
      break 
    end
    job = "$I2C,"..tostring(n_cliente)..","..job.."\n"
    err, len_written = p:write(job)
    assert(e == rs232.RS232_ERR_NOERROR)
    operacao[n_cliente] = 2 -- esperando escravo
  end
  
  -- recebe dados da serial
  err, data_read, size = p:read(1,5000)
  if size > 0 then
    serial_buffer = serial_buffer..data_read
    -- processa pacote recebido
    if data_read == '\n' then      
      dados = split(serial_buffer, ",") --$I2C,1,529,3000304,DUCOID3630323437060916
      serial_buffer = ""
      id_cliente = tonumber(dados[2])
      if status_conexoes[id_cliente] ~= nil then
        tempo_dec_s = tonumber(dados[4])/1000000
        duco_result = tonumber(dados[3])
        hashrate = math.floor( duco_result / tempo_dec_s)
      
        -- empacota e envia mensagem de resposta      
        local mens = tostring(tonumber(dados[3]))
        mens = mens..","
        mens = mens..tostring(hashrate)
        mens = mens..","..duino_coin_banner.." "..duino_coin_version..","..duino_coin_rig_id..","
        mens = mens..dados[5]
        mens = mens.."\n"
        conexoes[id_cliente]:send(mens)
        
        --trata retorno do servidor 
        local s,err = conexoes[id_cliente]:receive()
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
        operacao[id_cliente] = 1 -- solicitar job 
      end
    end
  end  
end
assert(p:close() == rs232.RS232_ERR_NOERROR)
--tcp:close()
