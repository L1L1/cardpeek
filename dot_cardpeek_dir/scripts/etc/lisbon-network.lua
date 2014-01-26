LISBON_NETWORK = {
	[01] = {
		["name"] = "Carris",
	},
	[02] = {
		["name"] = "Metro Lisboa",
		[4097] = { -- 0x1001
			["name"] = "Blue",
			[01] = "Amadora-Este",
			[02] = "Alfornelos",
			[03] = "Pontinha",
			[04] = "Carnide",
			[05] = "Colégio Militar/Luz",
			[06] = "Alto dos Moinhos",
			[07] = "Laranjeiras",
			[08] = "Jardim Zoológico",
			[09] = "Praça de Espanha",
			[10] = "São Sebastião",
			[11] = "Parque",
			[12] = "Marquês de Pombal",
			[13] = "Avenida",
			[14] = "Restauradores",
			[15] = "Terreiro do Paço",
			[16] = "Santa Apolónia"
		},
		[8194] = { -- 0x2002
			["name"] = "Yellow",
			[01] = "Rato",
			[02] = "Marquês de Pombal",
			[03] = "Picoas",
			[04] = "Saldanha",
			[05] = "Campo Pequeno",
			[06] = "Entrecampos",
			[07] = "Cidade Universitária",
			[08] = "Campo Grande",
			[09] = "Quinta das Conchas",
			[10] = "Lumiar",
			[11] = "Ameixoeira",
			[12] = "Senhor Roubado",
			[13] = "Odivelas"
		},
		[12291] = { -- 0x3003
			["name"] = "Green",
			[01] = "Cais do Sodré",
			[02] = "Baixa-Chiado",
			[03] = "Rossio",
			[04] = "Martim Moniz",
			[05] = "Intendente",
			[06] = "Anjos",
			[07] = "Arroios",
			[08] = "Alameda",
			[09] = "Areeiro",
			[10] = "Roma",
			[11] = "Alvalade",
			[12] = "Campo Grande",
			[13] = "Telheiras"
		},
		[16388] = { -- 0x4004
			["name"] = "Red",
			[01] = "São Sebastião",
			[02] = "Saldanha",
			[03] = "Alameda",
			[04] = "Olaias",
			[05] = "Bela Vista",
			[06] = "Chelas",
			[07] = "Olivais",
			[08] = "Cabo Ruivo",
			[09] = "Oriente",
			[10] = "Moscavide",
			[11] = "Encarnação",
			[12] = "Aeroporto"
		}
	},
	[03] = {
		["name"] = "CP",
		[4096] = { -- also 4098, 8192, 40962
			["name"] = "Sintra-Azambuja",
			[22] = "Rio de Mouro", --11 5
			[28] = "Agualva-Cacém", --14 7
			[31] = "Massamá-Barcarena", --15 7
			[34] = "Monte Abraão", --17 8
			[37] = "Queluz-Belas", --18 9
			[40] = "Amadora", --20 10
			[43] = "Reboleira", -- 21 10
			[49] = "Benfica", -- 24 12
			[52] = "Sete Rios/Entrecampos/Rossio/Alcântara-Terra/Campolide", -- 26 13
			[55] = "Roma-Areeiro", -- 27 13
			[64] = "Braço de Prata/Santa Apolónia", -- 32 16
			[70] = "Oriente", -- 35 17
			[88] = "Alverca", -- 44 22
			[112] = "Azambuja", -- 56 28
		},
		[40960] = {
			["name"] = "Sado/Cascais",
			-- Cascais
			[38] = "Cais do Sodré",
			[39] = "Santos",
			[40] = "Alcântara-Mar",
			[41] = "Belém",
			[42] = "Algés",
			[43] = "Cruz Quebrada",
			[44] = "Caxias",
			[45] = "Paço de Arcos",
			[46] = "Santo Amaro",
			[47] = "Oeiras",
			[48] = "Carcavelos",
			[49] = "Parede",
			[50] = "S. Pedro do Estoril",
			[51] = "S. João do Estoril",
			[52] = "Estoril",
			[53] = "Monte Estoril",
			[54] = "Cascais",
			-- Sado
			[55] = "Barreiro",
			[56] = "Barreiro-A",
			[57] = "Lavradio",
			[58] = "Baixa da Banheira",
			[59] = "Alhos Vedros",
			[60] = "Moita",
			[61] = "Penteado",
			[62] = "Pinhal Novo",
			[63] = "Venda do Alcaide",
			[64] = "Palmela",
			[65] = "Setúbal",
			[66] = "Praça do Quebedo (Setúbal)",
			[67] = "Praias do Sado-A"
		}
	},
	[04] = {
		["name"] = "Transtejo",
		[8192] = {
			["name"] = "Cais do Sodré - Cacilhas",
			[05] = "Cais do Sodré",
			[06] = "Cacilhas"
		}
	},
	[05] = {
		["name"] = "Transportes Sul do Tejo",
	},
	[06] = {
		["name"] = "Rodoviária de Lisboa",
		[193] = "PASSE RL"
	},
	[07] = {
		["name"] = "Soflusa",
		[00] = {
			["name"] = "Terreiro do Paço - Barreiro",
			[16] = "Barreiro"
		}
	},
	[08] = {
		["name"] = "Transportes Colectivos do Barreiro"
	},
	[09] = {
		["name"] = "Vimeca"
	},
	[13] = {
		["name"] = "Barraqueiro"
	},
	[15] = {
		["name"] = "Fertagus",
		[00] = {
			["name"] = "Lisboa - Setúbal",
			[01] = "Fogueteiro",
			[02] = "Foros de Amora",
			[03] = "Corroios",
			[04] = "Pragal",
			[05] = "Campolide",
			[06] = "Sete-Rios",
			[07] = "Entrecampos",
			[08] = "Roma-Areeiro",
			[09] = "Coina",
			[10] = "unknown/reserved",
			[11] = "Penalva",
			[12] = "Pinhal Novo",
			[13] = "Venda do Alcaide",
			[14] = "Palmela",
			[15] = "Setúbal"
		},
		[73] = "Ass. PAL - LIS",
		[193] = "Ass. FOG - LIS",
		[217] = "Ass. PRA - LIS"
	},
	[16] = {
		["name"] = "Metro Transportes do Sul",
		[01] = {
			["name"] = "1",
			[01] = "Cacilhas",
			[02] = "25 de Abril",
			[03] = "Gil Vicente",
			[04] = "S. João Baptista",
			[05] = "Almada",
			[06] = "Bento Gonçalves",
			[07] = "Cova da Piedade",
			[08] = "Parque da Paz",
			[09] = "António Gedeão",
			[10] = "Laranjeiro",
			[11] = "Santo Amaro",
			[12] = "Casa do Povo",
			[13] = "Corroios"
		},
		[02] = {
			["name"] = "2",
			[16] = "Pragal",
			[15] = "Ramalha",
			[07] = "Cova da Piedade",
			[08] = "Parque da Paz",
			[09] = "António Gedeão",
			[10] = "Laranjeiro",
			[11] = "Santo Amaro",
			[12] = "Casa do Povo",
			[13] = "Corroios"
		},
		[03] = {
			["name"] = "3",
			[01] = "Cacilhas",
			[02] = "25 de Abril",
			[03] = "Gil Vicente",
			[04] = "S. João Baptista",
			[05] = "Almada",
			[06] = "Bento Gonçalves",
			[15] = "Ramalha",
			[16] = "Pragal",
			[17] = "Boa Esperança",
			[18] = "Fomega",
			[19] = "Monte de Caparica",
			[20] = "Universidade"
		},
		[05] = "Passe MTS"
	},
	[30] = {
		["name"] = "multiple operators",
		[0113] = "Metro / RL 12",
		[0316] = "Vermelho A1",
		[0454] = "Metro/CP - R. Mouro/Meleças",
		[0720] = "Navegante urbano",
		[0725] = "Navegante rede",
		[0733] = "Navegante SL TCB Barreiro",
		[1088] = "Fertagus PAL - LIS + ML"
	},
	[31] = {
		["name"] = "multiple operators",
		[33592] = "Zapping"
	}
}

TRANSITION_LIST = {  
	[1] = "entry",  
	[4] = "exit"
}

PERIOD_UNITS = {
	[265] = "days",
	[266] = "months",
	[3600] = nil
}

SALES_POINT = {
	[01] = "Metro Lisboa",
	[02] = "Metro Lisboa",
	[03] = "Metro Lisboa",
	[06] = "Rodoviária de Lisboa",
	[08] = "Vimeca or Transtejo",
	[09] = "Transtejo",
	[10] = "Fertagus",
	[11] = "CP or MTS",
	[12] = "CP",
	[14] = "CP",
	[23] = "Multibanco",
	[24] = "Multibanco"
}
