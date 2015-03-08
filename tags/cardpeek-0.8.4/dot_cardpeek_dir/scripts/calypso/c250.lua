--
-- This file is part of Cardpeek, the smartcard reader utility.
--
-- Copyright 2009-2011 by 'L1L1'
--
-- Cardpeek is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- Cardpeek is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with Cardpeek.  If not, see <http://www.gnu.org/licenses/>.
--

-------------------------------------------------------------------------
-- *PLEASE NOTE*
-- This work is based on:
-- * public information about the calypso card specification, 
-- * partial information found on the web about the ticketing data 
--   format, as described in the French "intercode" documentation.
-- * experimentation and guesses, 
-- This information is incomplete. If you have further data, such 
-- as details of ISO 1545 or calypso card specs, please help send them
-- to L1L1@gmx.com
--------------------------------------------------------------------------

require('lib.strict')
require('lib.en1545')
require('lib.calypso_card_num')

----------------------------

intercode_BestContracts = {
  [0] = { en1545_REPEAT, 4, "BestContractCount", {
    [0] = { en1545_BITMAP, 3, "BestContract", {
      [0] = { en1545_NETWORKID, 24, "BestContractsNetworkId" },
      [1] = { en1545_UNDEFINED, 16, "BestContractsTariff" },
      [2] = { en1545_UNDEFINED, 5, "BestContractsPointer" }
    }}
  }}
}

intercode_Env = {
  [0] = { en1545_UNDEFINED, 6, "EnvVersionNumber" },
  [1] = { en1545_BITMAP, 7, "Env",{
    [0] = { en1545_NETWORKID, 24, "EnvNetworkId" },
    [1] = { en1545_UNDEFINED, 8, "EnvApplicationIssuerId" },
    [2] = { en1545_DATE, 14, "EnvApplicationValidityEndDate" },
    [3] = { en1545_UNDEFINED, 11, "EnvPayMethod" },
    [4] = { en1545_UNDEFINED, 16, "EnvAuthenticator" },
    [5] = { en1545_UNDEFINED, 32, "EnvSelectList" },
    [6] = { en1545_UNDEFINED, 2, "EnvData",
      [0] = { en1545_UNDEFINED, 1, "EnvCardStatus" },
      [1] = { en1545_UNDEFINED, 0, "EnvExtra" },
    }}
  }
}

intercode_Holder = {
  [0] = { en1545_BITMAP, 8, "Holder", {
    [0] = { en1545_BITMAP, 2, "HolderName", {
      [0] = { en1545_UNDEFINED, 85, "HolderSurname" },
      [1] = { en1545_UNDEFINED, 85, "HolderForename" }
    }},
    [1] = { en1545_BITMAP, 2, "HolderBirth", {
      [0] = { en1545_BCD_DATE, 32, "HolderBirthDate" },
      [1] = { en1545_UNDEFINED, 115, "HolderBirthPlace"}
    }},
    [2] = { en1545_UNDEFINED, 85, "HolderBirthName" },
    [3] = { en1545_UNDEFINED, 32, "HolderIdNumber" },
    [4] = { en1545_UNDEFINED, 24, "HolderCountryAlpha" },
    [5] = { en1545_UNDEFINED, 32, "HolderCompany" },
    [6] = { en1545_REPEAT, 4, "HolderProfiles(0..4)", {
      [0] = { en1545_BITMAP, 3, "Profile", {
      	[0] = { en1545_UNDEFINED, 24, "NetworkId" },
	[1] = { en1545_UNDEFINED, 8, "ProfileNumber" },
	[2] = { en1545_DATE, 14, "ProfileDate" }
	}}
    }},
    [7] = { en1545_BITMAP, 12, "HolderData", {
      [0] = { en1545_UNDEFINED, 4, "HolderDataCardStatus" },
      [1] = { en1545_UNDEFINED, 4, "HolderDataTeleReglement" },
      [2] = { en1545_UNDEFINED, 17, "HolderDataResidence" },
      [3] = { en1545_UNDEFINED, 6, "HolderDataCommercialID" },
      [4] = { en1545_UNDEFINED, 17, "HolderDataWorkPlace" },
      [5] = { en1545_UNDEFINED, 17, "HolderDataStudyPlace" },
      [6] = { en1545_UNDEFINED, 16, "HolderDataSaleDevice" },
      [7] = { en1545_UNDEFINED, 16, "HolderDataAuthenticator" },
      [8] = { en1545_UNDEFINED, 14, "HolderDataProfileStartDate1" },
      [9] = { en1545_UNDEFINED, 14, "HolderDataProfileStartDate2" },
      [10] = { en1545_UNDEFINED, 14, "HolderDataProfileStartDate3" },
      [11] = { en1545_UNDEFINED, 14, "HolderDataProfileStartDate4" }
    }}
  }}
}


intercode_Contract = {
  [0] = { en1545_BITMAP, 20, "Contract",
    {
      [0] = { en1545_NETWORKID, 24, "ContractNetworkId" },
      [1] = { en1545_UNDEFINED,  8, "ContractProvider" },
      [2] = { en1545_UNDEFINED, 16, "ContractTariff" },
      [3] = { en1545_UNDEFINED, 32, "ContractSerialNumber" },
      [4] = { en1545_BITMAP,  2, "ContractCustomerInfo", {
	[0] = { en1545_UNDEFINED,  6, "ContractCustomerProfile" },
	[1] = { en1545_UNDEFINED, 32, "ContractCustomerNumber" },
      }},
      [5] = { en1545_BITMAP,  2, "ContractPassengerInfo", {
	[0] = { en1545_UNDEFINED,  6, "ContractPassengerClass" },
	[1] = { en1545_UNDEFINED, 32, "ContractPassengerTotal" },
      }},
      [6] = { en1545_UNDEFINED, 6, "ContractVehiculeClassAllowed" },
      [7] = { en1545_UNDEFINED, 32, "ContractPaymentPointer" },
      [8] = { en1545_UNDEFINED, 11, "ContractPayMethod" },
      [9] = { en1545_UNDEFINED, 16, "ContractServices" },
      [10] = { en1545_AMOUNT, 16, "ContractPriceAmount" },
      [11] = { en1545_UNDEFINED, 16, "ContractPriceUnit" },
      [12] = { en1545_BITMAP, 7, "ContractContractRestriction", {
	[0] = { en1545_TIME, 11, "ContractStartTime" },
	[1] = { en1545_TIME, 11, "ContractEndTime" },
	[2] = { en1545_UNDEFINED, 8, "ContractRestrictDay" },
	[3] = { en1545_UNDEFINED, 8, "ContractRestrictTimeCode" },
	[4] = { en1545_UNDEFINED, 8, "ContractRestrictCode" },
	[5] = { en1545_UNDEFINED, 16, "ContractRestrictProduct" },
	[6] = { en1545_UNDEFINED, 16, "ContractRestrcitLocation" },
      }},
      [13] = { en1545_BITMAP, 9, "ContractContractValidityInfo", {
	[0] = { en1545_DATE, 14, "ContractStartDate" },
	[1] = { en1545_TIME, 11, "ContractStartTime" },
	[2] = { en1545_DATE, 14, "ContractEndDate" },
	[3] = { en1545_TIME, 11, "ContractEndTime" },
	[4] = { en1545_UNDEFINED, 8, "ContractDuration" },
	[5] = { en1545_DATE, 14, "ContractLimitDate" },
	[6] = { en1545_ZONES, 8, "ContractZones" },
	[7] = { en1545_UNDEFINED, 16, "ContractJourneys" },
	[8] = { en1545_UNDEFINED, 16, "ContractPeriodJourneys" },
      }},
      [14] = { en1545_BITMAP, 8, "ContractContractJourneyData", {
	[0] = { en1545_UNDEFINED, 16, "ContractOrigin" },
	[1] = { en1545_UNDEFINED, 16, "ContractDestination" },
	[2] = { en1545_UNDEFINED, 16, "ContractRouteNumbers" },
	[3] = { en1545_UNDEFINED, 8, "ContractRouteVariants" },
	[4] = { en1545_UNDEFINED, 16, "ContractRun" },
	[5] = { en1545_UNDEFINED, 16, "ContractVia" },
	[6] = { en1545_UNDEFINED, 16, "ContractDistance" },
	[7] = { en1545_UNDEFINED, 8, "ContractInterchange" },
      }},
      [15] = { en1545_BITMAP, 4, "ContractContractSaleData", {
	[0] = { en1545_DATE, 14, "ContractDate" },
	[1] = { en1545_TIME, 11, "ContractTime" },
	[2] = { en1545_UNDEFINED, 8, "ContractAgent" },
	[3] = { en1545_UNDEFINED, 16, "ContractDevice" },
      }},
      [16] = { en1545_UNDEFINED, 8, "ContractStatus" },
      [17] = { en1545_UNDEFINED, 16, "ContractLoyalityPoints" },
      [18] = { en1545_UNDEFINED, 16, "ContractAuthenticator" },
      [19] = { en1545_UNDEFINED, 0, "Contract"},
    }
  }
}

intercode_Event = {
  [0] = { en1545_DATE, 14, "EventDate" },
  [1] = { en1545_TIME, 11, "EventTime" },
  [2] = { en1545_BITMAP, 28, "Event",
    {
      [0] = { en1545_UNDEFINED,  8, "EventDisplayData" },
      [1] = { en1545_NETWORKID, 24, "EventNetworkId" },
      [2] = { en1545_UNDEFINED,  8, "EventCode" },
      [3] = { en1545_UNDEFINED,  8, "EventResult" },
      [4] = { en1545_UNDEFINED,  8, "EventServiceProvider" },
      [5] = { en1545_UNDEFINED,  8, "EventNotOkCounter" },
      [6] = { en1545_UNDEFINED, 24, "EventSerialNumber" },
      [7] = { en1545_UNDEFINED, 16, "EventDestination" },
      [8] = { en1545_UNDEFINED, 16, "EventLocationId" },
      [9] = { en1545_UNDEFINED,  8, "EventLocationGate" },
      [10] = { en1545_UNDEFINED, 16, "EventDevice" },
      [11] = { en1545_NUMBER, 16, "EventRouteNumber" },
      [12] = { en1545_UNDEFINED,  8, "EventRouteVariant" },
      [13] = { en1545_UNDEFINED, 16, "EventJourneyRun" },
      [14] = { en1545_UNDEFINED, 16, "EventVehiculeId" },
      [15] = { en1545_UNDEFINED,  8, "EventVehiculeClass" },
      [16] = { en1545_UNDEFINED,  5, "EventLocationType" },
      [17] = { en1545_UNDEFINED,240, "EventEmployee" },
      [18] = { en1545_UNDEFINED, 16, "EventLocationReference" },
      [19] = { en1545_UNDEFINED,  8, "EventJourneyInterchanges" },
      [20] = { en1545_UNDEFINED, 16, "EventPeriodJourneys" },
      [21] = { en1545_UNDEFINED, 16, "EventTotalJourneys" },
      [22] = { en1545_UNDEFINED, 16, "EventJourneyDistance" },
      [23] = { en1545_AMOUNT, 16, "EventPriceAmount" },
      [24] = { en1545_UNDEFINED, 16, "EventPriceUnit" },
      [25] = { en1545_UNDEFINED,  5, "EventContractPointer" },
      [26] = { en1545_UNDEFINED, 16, "EventAuthenticator" },
      [27] = { en1545_UNDEFINED,  5, "EventBitmapExtra" },
    }
  }
}

en1545_map(CARD,"Environment",intercode_Env,intercode_Holder)
en1545_map(CARD,"Event logs",intercode_Event)
en1545_map(CARD,"Special events",intercode_Event)
en1545_map(CARD,"Contract list",intercode_BestContracts)
	-- FIXME: here we should parse the "contracts" according to "Tariff"
	--        but for now we will assume that contract format is 'FF' 
	--        as defined in intercode
en1545_map(CARD,"Contracts",intercode_Contract)

calypso_card_num()
