 ** This program contains proprietary information which is a trade            **
 ** secret of ALCATEL TELECOM and also is protected as an unpublished         **
 ** work under applicable Copyright laws. Recipient is to retain this         **
 ** program in confidence and is not permitted to use or make copies          **
 ** thereof other than as permitted in a written agreement with               **
 ** ALCATEL TELECOM.                                                          **
 **                                                                           **
 ******************************************************************************/

/****************************** IDENTIFICATION ********************************
 **
 ** Project        : ISAM R3.7.10
 **
 ** Module         :  Port mapping
 **
 ** File name      : PortMapping.c
 **
 ** Creation date  : 11 Oct. 2004
 **
 ** Description    : Translate functions from IPD port to porttype, identifier,
 rack, shelf and slot flavours
 **
 ** Reference(s)   :
 **
 ** contact        : Jean Samoise (Jean.Samoise@alcatel-lucent.com)
 **
 ******************************************************************************/


#include "PortMappingImpl.hpp"
#include <InfraMgntNt/PortMapping.hpp>

#include <debug/trace.h>


#include <SfpMgnt/IocmClassIds.hpp>
#include <SfpMgnt/SfpMgntTables_ifc.hh>
#include <SfpMgnt/HostSfpConfigTableHelper.h>

extern "C"
{
#include <AsamTrap/asam_trap.h>
}


#ifdef TARG_ARCH_HOST
#define LINEFEED "\r\n"
#else
#define LINEFEED "\n"
#endif

#include <ConfigDataXml/LanxCfgProxy.hpp>

#include <ConfigDataXml/SfpPortMapProxy.hpp>
#include <EqptCoreCommon/OwnEqptIdentification.hh>
#include <EqptCoreNt/OwnEqptIdentificationNt.hh>
#include <EqptCoreCommon/IndexConversionCommon.hh>
#include <EqptLib/BoardInfo.hpp>

#include <EqptCoreNt/IndexConversion.hh>
#include <CapabilityMgr/CapabilityMgr.hpp>
//#include <EqptCoreNt/mapper/eqptCoreNtMan_proto.h>
#include <InfraNt/interfaces/portMap_proto.h>
#include <common_types/com_types.h>

#include <stdio.h>
#include <TaskConverter/TaskConvert.hpp>

#define PRTMLabel "PRTM"
// the following is removed for ALU00807075 
// #define MAX_IPDPORTS 48
// and replaced by
#define MAX_IPDPORTS 50
// end of addition for ALU00807075 

#define ISLAGNUMBER 0x50000000
#define LAGIDMASK   0x7F

using namespace InfraMgntNt ;




boolean PortMappingImpl::portDataIsInitialised = FALSE ;
unsigned long  PortMappingImpl::traceId_m = 0 ;
map<int, PortMappingImpl::IPDPORTDATA> PortMappingImpl::portToIpdPortData ;
map<unsigned long, int> PortMappingImpl::ipdPortDataToPort ;
map<int,bool> PortMappingImpl::remoteLTs_m ;
PortMapping::SlotAddrMode PortMappingImpl::addressingMode_m ;
unsigned int  PortMappingImpl::configurationChanges_m ;

typedef enum {
        E_NOT_APPLICABLE = 0,
        E_SFP = 1,
        E_XFP = 2,
        E_ETH= 3,
        E_CFP = 4
} lanxCfgPhyIfType_e;


typedef enum {
        E_INVALID_PORT = 0,
        E_ASAM_PORT = 1,
        E_CONTROL_PORT = 2,
        E_OTHER_PORT = 3
} lanxCfgPortType_e;

unsigned short PortMappingImpl::convertPorttype(unsigned short chipCap, unsigned short porttypeFromLanxCfg, unsigned short sfpNr, unsigned short slotId)

{
        unsigned short returnValue = porttypeFromLanxCfg ;
        if ((chipCap >> 1) == 0) // for reuse Port Mapping for minint like nt's
        {
                switch (porttypeFromLanxCfg)
                {
                        case E_INVALID_PORT :
                                returnValue = PortMapping::E_INVALID ;
                                break ;
                        case E_ASAM_PORT:
                                if (sfpNr == 0)
                                {
                                        if ((EqptCore::OwnEqptIdentification::getOwnSlotId() == slotId) || (EqptCoreNt::OwnEqptIdentificationNt::getPeerLogSlotID() == slotId))
                                        {
                                                returnValue = PortMapping::E_PHYSICAL_PORT ;
                                        }
                                        else
                                        {
                                                returnValue = PortMapping::E_PORT_LT_LAG ;
                                        }
                                }
                                else
                                {
                                        if ((EqptCore::OwnEqptIdentification::getOwnSlotId() == slotId) || (EqptCoreNt::OwnEqptIdentificationNt::getPeerLogSlotID() == slotId))
                                        {
                                                returnValue = PortMapping::E_PHYSICAL_PORT ;
                                        }
                                        else
                                        {
                                                returnValue =  PortMapping::E_PORT_IO_LAG ;
                                        }
                                }
                                break ;
                        case E_CONTROL_PORT:
                                returnValue = PortMapping::E_PORT_CONTROL ;
                                break ;
                        case E_OTHER_PORT:
                                if (slotId == 0) 
                                {
                                        returnValue = PortMapping::E_INVALID ;   
                                }
                                else
                                        if ((EqptCore::OwnEqptIdentification::getOwnSlotId() == slotId) || (EqptCoreNt::OwnEqptIdentificationNt::getPeerLogSlotID() == slotId))
                                        {
                                                returnValue = PortMapping::E_PHYSICAL_PORT ;
                                        }
                                        else
                                        {
                                                returnValue = PortMapping::E_PORT_IO_LAG ;
                                        }
                                break ;
                        default :
                                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                                "Could not convert porttype from LanxCfgData to internal used porttype (%d)in the PortMapping",porttypeFromLanxCfg, 0);
                }
        }
        return returnValue ;
}


boolean PortMappingImpl::isTupleOfLanxCfgTableForMe(short sta,short osma, short ntcca, short profa, short stb ,short osmb, short ntccb, short profb)

{
        return ((sta == stb) &&   (osma == osmb) && ( ntcca == ntccb) && (profa == profb)) ;
}

bool  PortMappingImpl::isTupleOfSfpPortMappingTableForMe(
                Eqpt::ShelfType &  sta, Eqpt::BoardType&  bta, Eqpt::ProfileId prfida, short sfpnoa,
                const Eqpt::ShelfType &  stb , const Eqpt::BoardType&  btb, Eqpt::ProfileId prfidb, short sfpnob)

{
        return ((sta == stb) &&   (bta == btb) && ( prfida == prfidb) && ( sfpnoa == sfpnob) ) ;
}

int  PortMappingImpl::getPortType(int sfpno, SfpMgnt::HostSfpConfigTable_ifc *hostSfpConfigTable , int defaultPortType, Sfp::HostConfigEntry        &    entry_l)
{
        int returnValue = defaultPortType ;
        // Sfp::HostConfigEntry  entry_l ;
        if (hostSfpConfigTable != 0)
        {
                IOCM::Status iocmStatus_l ;
                SfpMgnt::OpResult  opResult_l = SfpMgnt::OpResult_ok;
                iocmStatus_l = SfpMgnt::HostSfpConfigTableHelper::getEntryLegacy(*hostSfpConfigTable,
                                sfpno,
                                entry_l,
                                opResult_l);
                if(iocmStatus_l != IOCM::Status_OK)
                {
                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_NON_RECOV,
                                        PRTMLabel, __FILE__, __LINE__,
                                        "IOCM invocation failed - getEntry: %#x", iocmStatus_l);
                }
                else if(opResult_l != SfpMgnt::OpResult_ok) // NTIO not planned
                {
                        //      lxmtOstream << "No SFP configuration: " << opResult_l << endl;
                        returnValue  = defaultPortType;
                }
                else
                {
                        returnValue = (entry_l.isUplink)? E_OTHER_PORT : E_ASAM_PORT;
                }
        }
        return returnValue ;

}

AsamTypes::ErrorCode PortMappingImpl::getTypeBasedSlotPosition(
                int slotData, EqptCore::SlotNumberingSchemeInfo& infoSlotTypeBased)
{
        AsamTypes::ErrorCode returnValue =  EqptCore::eqptNoError;
        infoSlotTypeBased.type_based_info.rack =0 ;
        infoSlotTypeBased.type_based_info.shelf =0 ;
        infoSlotTypeBased.type_based_info.slotPos =0 ;
        returnValue = EqptCoreNt::IndexConversion::GetSlotPositionBasedOnNumberingScheme(slotData,
                        EqptCore::e_type_based,
                        infoSlotTypeBased,
                        Eqpt::NOTgroup) ;
        if (returnValue != EqptCore::eqptNoError)
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                "Could not get type based slot id (%d)in the PortMapping",
                                slotData, 0);
        }
        return returnValue ;

}

AsamTypes::ErrorCode PortMappingImpl::getPositionBasedSlotPosition(
                int slotData, EqptCore::SlotNumberingSchemeInfo& infoSlotPositionBased )

{
        AsamTypes::ErrorCode returnValue = EqptCore::eqptNoError ;
        infoSlotPositionBased.pos_based_info.rack =0 ;
        infoSlotPositionBased.pos_based_info.shelf =0 ;
        infoSlotPositionBased.pos_based_info.slotPos =0 ;
        returnValue =  EqptCoreNt::IndexConversion::GetSlotPositionBasedOnNumberingScheme(slotData,
                        EqptCore::e_position_based,
                        infoSlotPositionBased,
                        Eqpt::NOTgroup)  ;

        if (returnValue != EqptCore::eqptNoError)
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                "Could not get position based slot id (%d)in the PortMapping",
                                slotData, 0);
        }

        return returnValue ;
}

tInt32 PortMapping::port2Identifier(tPortId tmnxPortId,
                IpdPortBoardType *board,
                tUint32 *rack,
                tUint32 *shelf,
                tUint32 *slot,
                IpdPortCliType *porttype,
                tUint32 *identifier,
                SlotAddrMode addressingMode)

{
        unsigned int ipd_port =  InfraMgntNt::PortMappingImpl::makePortIdFromTmnxPortId(tmnxPortId) ;
        tInt32 returnValue = E_FAILURE;

        if ((PortMappingImpl::portDataIsInitialised == TRUE) &&
                        (board != 0) &&
                        (rack != 0) &&
                        (shelf != 0) &&
                        (slot != 0) &&
                        (porttype != 0) &&
                        (identifier != 0))
        {

                returnValue =  E_NOT_PRESENT ;
                if ((tmnxPortId>>8)  == ISLAGNUMBER >> 8)
                {
                        unsigned long strippedPortId = tmnxPortId - ISLAGNUMBER;
                        if (PortMappingImpl::isValidLagId(strippedPortId))
                        {
                                *rack = 0;
                                *shelf = 0;
                                *slot = 0;
                                *identifier = strippedPortId;
                                *porttype = IPD_PORT_CLI_LAG;
                                *board = IPD_PORT_INVALID_BOARD;
                                returnValue = E_SUCCESS;

                        }
                }
                if ((returnValue == E_NOT_PRESENT) && (PortMappingImpl::portToIpdPortData.find(ipd_port) != PortMappingImpl::portToIpdPortData.end()))
                {

                        returnValue = E_SUCCESS;
                        PortMappingImpl::IPDPORTDATA pd = PortMappingImpl::portToIpdPortData[ipd_port] ;
                        if ((pd.slotId > 1) == 0)
                        {
                                *rack = 0;
                                *shelf = 0;
                                *slot = 0;
                        }
                        else

                        {
                                if (addressingMode == POSITION_BASED_ADDR_MODE)
                                {
                                        EqptCore::SlotNumberingSchemeInfo infoSlotPositionBased  ;
                                        PortMappingImpl::getPositionBasedSlotPosition(pd.slotId, infoSlotPositionBased) ;
                                        *rack = infoSlotPositionBased.pos_based_info.rack ;
                                        *shelf = infoSlotPositionBased.pos_based_info.shelf ;
                                        *slot = infoSlotPositionBased.pos_based_info.slotPos ;
                                }
                                else
                                {
                                        EqptCore::SlotNumberingSchemeInfo infoSlotTypeBased  ;
                                        PortMappingImpl::getTypeBasedSlotPosition(pd.slotId, infoSlotTypeBased) ;
                                        *rack = infoSlotTypeBased.type_based_info.rack ;
                                        *shelf = infoSlotTypeBased.type_based_info.shelf ;
                                        *slot = infoSlotTypeBased.type_based_info.slotPos ;
                                }
                        }

                        *identifier = pd.identifier ;
                        *porttype = pd.portType ;
                        *board = pd.boardtype ;
                }
        }
        else
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                "IPD port mapping not initialised or target parameter is null.") ;
        }
        return returnValue ;
}

tInt32 PortMapping::getLTPortId(int & ipdportId, tUint32 slotid)

{
        tInt32 returnValue = E_FAILURE ;
        ipdportId = -1 ;
        PortMappingImpl::IPDPORTDATA pd ;

        pd.identifier = 0 ;
        pd.boardtype = IPD_PORT_BOARD_LT ;
        pd.slotId  = slotid ;
        pd.portType = (IpdPortCliType)0 ;
        long  index = PortMappingImpl::hashPortData(pd) ;
        if ( PortMappingImpl::ipdPortDataToPort.find(index) !=  PortMappingImpl::ipdPortDataToPort.end())
        {
                ipdportId =  PortMappingImpl::ipdPortDataToPort[index] ;
                returnValue = E_SUCCESS ;
        }
        else
        {
                returnValue = E_FAILURE ;
        }
        return returnValue ;
}


// TODO verify !!!
bool PortMapping::getLTPortIdTemp(Eqpt::LogSlotId slotId_i,
                unsigned int& ltPortId_o)
{

        tUint32 slot_l = static_cast<tUint32>(slotId_i);
        int port_l;

        InfraMgntNt::tInt32 result_l = getLTPortId(port_l,slot_l);

        ltPortId_o = static_cast<unsigned int>(port_l);

        return (result_l == E_SUCCESS);

}


tInt32 PortMapping::getFacePlateNumberForRemoteLt(int ipdportid, int & facePlateNumber)
{
        tInt32 returnValue = E_FAILURE ;
        if ( PortMappingImpl::portToIpdPortData.find(ipdportid) !=  PortMappingImpl::portToIpdPortData.end())
        {
                if (PortMappingImpl::portToIpdPortData[ipdportid].boardtype == IPD_PORT_BOARD_LT)
                {
                        tUint32 phyPortSlot ;
                        tUint32 phyItfType ;
                        Eqpt::LogSlotId lsmSlotId ;
                        tUint32 logPortType ;
                        return getPortMapping(ipdportid, &phyPortSlot, (tUint32 *)&facePlateNumber, &phyItfType, &lsmSlotId,&logPortType) ;
                }
        }
        return returnValue ;
}

tInt32 PortMapping::getPortIdForFacePlateNumber(int & ipdportid, int logSlotId, int  facePlateNumber, IpdPortCliType portType)
{
        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"getPortIdForFacePlateNumber called with logslot = %d faceplate number = %d, porttype = %d%s",
                        logSlotId, facePlateNumber, portType, LINEFEED) ;

        tInt32 returnValue = E_FAILURE ;
        for (int i = 1 ; i < MAX_IPDPORTS; i++)
        {
                PortMappingImpl::IPDPORTDATA pd = PortMappingImpl::portToIpdPortData[i] ;
#ifdef FX_SHELF 
                if (((pd.slotId == logSlotId) || ((pd.slotId == 0) && (logSlotId == 4481))) &&
                                ((pd.identifier == facePlateNumber) || (pd.phyPortNbr == facePlateNumber)) &&
                                (pd.phyPortType == portType + 1))
#else
                        if (((pd.slotId == logSlotId) || ((pd.slotId == 0) && (logSlotId == 4352))) &&
                                        ((pd.identifier == facePlateNumber) || (pd.phyPortNbr == facePlateNumber)) &&
                                        (pd.phyPortType == portType + 1))
#endif
                        {
                                returnValue = E_SUCCESS ;
                                ipdportid = i ;
                                break ;
                        }
        }

        return returnValue ;
}


tInt32 PortMapping::getNTxPortId(int & ipdportid, tUint32 slotid, IpdPortCliType porttype, tUint32 identifier)

{
        tInt32 returnValue = E_FAILURE ;
        ipdportid = -1 ;
        IpdPortBoardType board = IPD_PORT_BOARD_NTA ;
        switch (slotid)
        {
#ifdef FX_SHELF
                case 4481 :
#else
                case 4352 :
#endif
                        board = IPD_PORT_BOARD_NTIO1 ;
                        returnValue = E_SUCCESS ;
                        break ;
                case 4353 :
                        board = IPD_PORT_BOARD_NTA ;
                        returnValue = E_SUCCESS ;
                        break ;
                case 4354 :
                        board = IPD_PORT_BOARD_NTB ;
                        returnValue = E_SUCCESS ;
                        break ;
                default :
                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                        "A not NTx board was presented to IgetNTxPortId (slot id = %d)", slotid) ;

        }
        if (returnValue == E_SUCCESS)
        {
                if ((porttype == IPD_PORT_CLI_SFP) ||
                                (porttype == IPD_PORT_CLI_XFP) ||
                                (porttype == IPD_PORT_CLI_ETH) ||
                                (porttype == IPD_PORT_CLI_AI))
                {
                        tPortId tmnxipdPort ;
                        if (identifier2port(&tmnxipdPort,
                                                board,
                                                0, 0, 0, // Don't care here
                                                porttype,
                                                identifier,POSITION_BASED_ADDR_MODE, 0 ) == E_SUCCESS)
                        {
                                ipdportid = tPortId_to_logPort(tmnxipdPort) ;
                        }
                        else
                        {
                                returnValue = E_FAILURE ;
                                //      err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                //                      "identifier2port for an NTx board failed.") ;
                        }
                }
                else
                {
                        returnValue = E_FAILURE ;
                        //err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                        //              "Invalid porttype was presented to getNtxPortId (should be %d or %d and is actually %d.",IPD_PORT_CLI_SFP, IPD_PORT_CLI_XFP, porttype) ;
                }
        }
        return returnValue ;
}


// TODO verify this new method !!!
bool PortMapping::getNTxPortIdTemp(
                Eqpt::LogSlotId slotId_i,
                IpdPortCliTypeTemp portType_i,
                Eqpt::Port faceplatePortNr_i,
                unsigned int& ntPortId_o)
{

        // Map parameters to PortMapping types
        tUint32 identifier_l = static_cast<tUint32>(faceplatePortNr_i);
        tUint32 slot_l = static_cast<tUint32>(slotId_i);
        IpdPortCliType portType_l = static_cast<IpdPortCliType>(portType_i);
        int ntPort_l;

        // Call existing conversion method with PortMapping types
        InfraMgntNt::tInt32 result =
                getNTxPortId(ntPort_l, slot_l, portType_l, identifier_l);

        ntPortId_o = static_cast<unsigned int>(ntPort_l);

        return (result == E_SUCCESS);

}


tInt32  PortMapping::identifier2port(
                tPortId *tmnxipdPort,
                PortMapping::IpdPortBoardType board,
                tUint32 rack,
                tUint32 shelf,
                tUint32 slot,
                PortMapping::IpdPortCliType porttype,
                tUint32 identifier,
                PortMapping::SlotAddrMode addressingMode,
                tUint32 cliConfigSpeed)
{
        unsigned int ipdPort = 0 ;
        tInt32 returnValue = E_FAILURE ;
        PortMappingImpl::IPDPORTDATA portData ;
        EqptCore::SlotNumberingSchemeInfo i_slot_position_info ;
        Eqpt::LogSlotId slotId = EqptCore::LogSlotIdInt::makeLogSlotId(rack + 1 , shelf + 1, slot + 3) ;
        const EqptCore::SlotNumberingScheme slotNumberingScheme = (addressingMode == POSITION_BASED_ADDR_MODE) ? EqptCore::e_position_based : EqptCore::e_type_based ;
        unsigned long index  ;
        AsamTypes::ErrorCode errorCode = EqptCore::eqptNoError;

        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"identifier2port called with boardtype = %d, rack= %d, shelf = %d, slot = %d, porttype = %d, identifier = %d addressingmode = %d, cliConfigSpeed = %d.%s",
                        board, rack, shelf, slot, porttype, identifier, addressingMode, cliConfigSpeed, LINEFEED) ;


        assert(tmnxipdPort) ;

        portData.identifier = identifier ;
        portData.portType = porttype ;
        portData.boardtype = board ;

        switch(board)
        {
                case IPD_PORT_INVALID_BOARD :
                        if (porttype == IPD_PORT_CLI_LAG)
                        {

                                if (PortMappingImpl::isValidLagId(identifier))
                                {
                                        *tmnxipdPort = ISLAGNUMBER + identifier ;
                                        returnValue = E_SUCCESS ;
                                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Lag id id =%d%s", *tmnxipdPort , LINEFEED) ;

                                }
                                else
                                {
                                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Invalid lag id =%d%s", identifier, LINEFEED) ;
                                }
                        }
                        else
                        {
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Expected a cli lag porttype with an invalid board. porttype  =%d, identifier =%d%s",
                                                porttype,
                                                identifier, LINEFEED) ;
                        }
                        return returnValue ;

                case IPD_PORT_BOARD_ACU :
                        if ((porttype == IPD_PORT_CLI_100M) && (identifier == 1))
                        {
                                ipdPort = 5 ;
                                slotId = 0 ;
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Acu board presented to identifier2port.%s", LINEFEED) ;
                                returnValue = E_SUCCESS ;
                        }
                        else
                        {
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                                "Invalid parameters presented to identifier2port for the board type = %d , portType = %d, identifier = %d%s",
                                                board, porttype,
                                                identifier, LINEFEED) ;
                        }
                        break ;
                case IPD_PORT_BOARD_NT :
                        if (( ((porttype == IPD_PORT_CLI_VP) || (porttype == IPD_PORT_CLI_UGW )) && (identifier == 1) )
                                        || (porttype == IPD_PORT_CLI_MCAST))
                        {
                                ipdPort = 1 ;
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"nt board presented to identifier2port.%s", LINEFEED) ;
                                slotId = 0 ;
                                returnValue = E_SUCCESS ;
                        }
                        else
                        {
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                                "Invalid parameters presented to identifier2port for the board type = %d , portType = %d, identifier = %d%s",
                                                board, porttype,
                                                identifier, LINEFEED) ;
                        }
                        break ;

                case IPD_PORT_BOARD_NTIO2 :
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                        "Not handled NTIO2 board presented to identifier2port!!!!!!!%s", LINEFEED) ;
                        slotId = 0 ;
                        break ;
                case IPD_PORT_BOARD_NTA :
                case IPD_PORT_BOARD_NTB :
                        if (porttype == IPD_PORT_CLI_CPORT)
                        {
                                if (identifier == 1)
                                {
                                        slotId = (board == IPD_PORT_BOARD_NTA) ? 0 : 1 ;
                                        returnValue = E_SUCCESS ;
                                }
                                else
                                {
                                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                                        "Invalid parameters presented to identifier2port for the board type = %d , portType = %d, identifier = %d%s",
                                                        board, porttype,
                                                        identifier, LINEFEED) ;
                                }
                        }
                        else
                        {
                                if ((porttype == IPD_PORT_CLI_XFP) || (porttype == IPD_PORT_CLI_SFP) || (porttype == IPD_PORT_CLI_ETH) || (porttype == IPD_PORT_CLI_CFP))
                                {
                                        slotId = ((board == IPD_PORT_BOARD_NTA) ? 4353 : 4354) ;
                                        returnValue = E_SUCCESS ;
                                }
                                else if (porttype == IPD_PORT_CLI_IEEE1588)
                                {
                                        if (identifier == 1)
                                        {
                                                slotId = ((board == IPD_PORT_BOARD_NTA) ? 4353 : 4354) ;
                                                returnValue = E_SUCCESS ;
                                        }
                                        else
                                        {
                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                                                "Invalid parameters presented to identifier2port for the board type = %d , portType = %d, identifier = %d%s",
                                                                board, porttype,
                                                                identifier, LINEFEED) ;
                                        }
                                }
                                //  the following is added for ALU00807075 
                                else if (porttype == IPD_PORT_CLI_AI)
                                {
                                        //to be checked: is identifier the number of that number of that type of port, for now always 1 as only 1 AI port per board 
                                        if (identifier == 1)
                                        {
                                                slotId = ((board == IPD_PORT_BOARD_NTA) ? 4353 : 4354) ;        //0x1101 = 4353
                                                returnValue = E_SUCCESS ;
                                        }
                                        else
                                        {
                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                                                "Invalid parameters presented to identifier2port for the board type = %d , portType = %d, identifier = %d%s",
                                                                board, porttype,
                                                                identifier, LINEFEED) ;
                                        }
                                }
                                // end of addition for ALU00807075 
                                else
                                {
                                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                                        "Invalid parameters presented to identifier2port for the board type = %d , portType = %d, identifier = %d%s",
                                                        board, porttype,
                                                        identifier, LINEFEED) ;
                                }
                        }
                        break ;

                case IPD_PORT_BOARD_NTIO1 :
                        returnValue = E_SUCCESS ;
                        if ((rack != 0) || (shelf != 0) || (slot !=0))
                        {
                                slotId = EqptCore::LogSlotIdInt::makeLogSlotId(rack + 1 , shelf + 1, slot + 3) ;
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Computed slot id for NTIO1 = %d.%s",slotId, LINEFEED) ;
                        }
                        else
                        {
                                slotId = 0 ;
                        }
                        break ;
                case IPD_PORT_BOARD_LT :
                        if (addressingMode == POSITION_BASED_ADDR_MODE)
                        {
                                i_slot_position_info.pos_based_info.rack = rack ;
                                i_slot_position_info.pos_based_info.shelf = shelf ;
                                i_slot_position_info.pos_based_info.slotPos = slot ;

                        }
                        else
                        {
                                i_slot_position_info.type_based_info.rack = rack ;
                                i_slot_position_info.type_based_info.shelf = shelf ;
                                i_slot_position_info.type_based_info.slotPos = slot ;
                        }
                        errorCode =  EqptCoreNt::IndexConversion::GetLogicalSlotIdBasedOnNumberingScheme(slotNumberingScheme,i_slot_position_info, slotId) ;
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Computed slot id for LT = %d.%s", slotId, LINEFEED) ;
                        returnValue = (errorCode ==  EqptCore::eqptNoError ? E_SUCCESS : E_FAILURE) ;
                        break ;
        }
        if (returnValue == E_SUCCESS)
        {
                {
                        portData.slotId = slotId ;
                        index = PortMappingImpl::hashPortData(portData) ;
                        if ( PortMappingImpl::ipdPortDataToPort.find(index) !=  PortMappingImpl::ipdPortDataToPort.end())
                        {
                                ipdPort =  PortMappingImpl::ipdPortDataToPort[index] ;
                                returnValue = E_SUCCESS ;
                        }
                        else
                        {
                                returnValue = E_FAILURE ;
                        }
                }

                *tmnxipdPort =   InfraMgntNt::PortMappingImpl::makeTmnxPortId(ipdPort) ;
        }
        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"identifier2port found ipdport = %d.%s",ipdPort, LINEFEED) ;
        return returnValue ;
}

bool PortMappingImpl::isValidLagId(tUint32 lagId)

{
        return (lagId <=  LAGIDMASK) ;
}


PortMapping::SlotAddrMode PortMappingImpl::getAddressingMode()

{
        return  addressingMode_m ;
}


void PortMappingImpl::setAddressingMode (PortMapping::SlotAddrMode slotAddrMode)

{
        addressingMode_m = slotAddrMode;
}

void PortMappingImpl::populateCableLsmData(tUint32 ipd_port,
                unsigned short hostSfpConfigEntry_l,
                PortMapping::IpdPortCliType  cliPortType,
                int identifier,
                SfpMgnt::HostSfpConfigTable_ifc * hostSfpConfigTable,
                InfraMgntNt::PortMapping::IpdPortBoardType boardType,
                int slotId)

{
        Sfp::HostConfigEntry            entry_l;
        int i = ipd_port ;
        int ptp =  PortMappingImpl::getPortType(hostSfpConfigEntry_l,  hostSfpConfigTable, PortMapping::E_INVALID, entry_l) ;
        if (ptp != PortMapping::E_INVALID)
        {
                portToIpdPortData[i].boardtype =  boardType;
                PortMappingImpl::portToIpdPortData[i].portType = cliPortType ;

                if (entry_l.isUplink  == false)
                {
                        portToIpdPortData[i].phyPortSlot = slotId ;
                        portToIpdPortData[i].phyPortNbr = identifier ;
                        if (portToIpdPortData[i].phyPortNbr > 256) // For XFP's
                        {
                                portToIpdPortData[i].phyPortNbr -= 256 ;
                        }
                        portToIpdPortData[i].phyPortType = cliPortType + 1;
                        portToIpdPortData[i].identifier = 0 ;
                        PortMappingImpl::portToIpdPortData[i].portType = (InfraMgntNt::PortMapping::IpdPortCliType)0 ;
                        if ((entry_l.plannedCabledLsm.rack != 255) && (entry_l.plannedCabledLsm.shelf  != 255) && (entry_l.plannedCabledLsm.slot  != 255))
                        {
                                portToIpdPortData[i].slotId =  EqptCore::LogSlotIdInt::makeLogSlotId(
                                                entry_l.plannedCabledLsm.rack + 1,
                                                entry_l.plannedCabledLsm.shelf + 1,
                                                entry_l.plannedCabledLsm.slot + 3);
                        }
                        else
                        {
                                portToIpdPortData[i].slotId = 0 ;
                        }

                        portToIpdPortData[i].boardtype = PortMapping::IPD_PORT_BOARD_LT ; // core 1 behavior
                        remoteLTs_m[portToIpdPortData[i].slotId] = true ;

                }
                else
                {
                        portToIpdPortData[i].phyPortType = cliPortType + 1;
                        portToIpdPortData[i].identifier = identifier ;
                        portToIpdPortData[i].slotId = slotId ;
                }
                ipdPortDataToPort[hashPortData( portToIpdPortData[i])] = i ;
        }
}

void PortMappingImpl::getCableLsmData(tUint32 ipd_port,unsigned short hostSfpConfigEntry_l, tUint32 *logPortType, Eqpt::LogSlotId * lsmSlotId, SfpMgnt::HostSfpConfigTable_ifc * hostSfpConfigTable)

{
        Sfp::HostConfigEntry            entry_l;


        int ptp = getPortType(hostSfpConfigEntry_l,  hostSfpConfigTable, PortMapping::E_INVALID, entry_l);
        if(ptp != PortMapping::E_INVALID)

        {
                if (entry_l.isUplink  == false)
                {
                        *logPortType = E_EqptPortMappingLogPortType_downlink;
                        if ((entry_l.plannedCabledLsm.rack == 0xFF) && (entry_l.plannedCabledLsm.shelf == 0xFF) &&
                                        (entry_l.plannedCabledLsm.slot == 0xFF))
                        {
                                *lsmSlotId = E_UNKNOWN;
                        }
                        else
                        {
                                *lsmSlotId = portToIpdPortData[ipd_port].slotId;
                        }
                }
                else
                {
                        *logPortType = E_EqptPortMappingLogPortType_uplink; //uplink (NETWORK_PORT_TYPE or UNUSED_PORT_TYPE)
                        *lsmSlotId = E_UNKNOWN;
                }
        }

}


unsigned long  PortMappingImpl::hashPortData(const  PortMappingImpl::IPDPORTDATA & pd)
{

        // Hashing RSS + portType + identifier
        // 16 bits for RSS, bit 17 - 20 for portType, 21-25 identifier
        unsigned long returnValue = 0 ;

        returnValue = pd.slotId  ;
        if ( pd.portType != PortMapping::IPD_PORT_CLI_INVALID )
        {
                returnValue += pd.portType << 16  ;
        }
        returnValue +=  pd.identifier << 20 ;
        return returnValue ;
}


void  PortMapping::initPortData()

{
        trc_register("PRTM", TRC_FLOW, &InfraMgntNt::PortMappingImpl::traceId_m) ;
        dbg_link("PortMap", InfraMgntNt::PortMappingImpl::showHelp, InfraMgntNt::PortMappingImpl::procCmd);
        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"initPortData of PortMapping.%s",LINEFEED) ;
        Eqpt::ShelfType shelfType = E_SHELF_NOT_PLANNED;
        EqptCore::OwnEqptIdentification::getOwnShelfType (shelfType);
        unsigned int ntChipCap = 1;
        Eqpt::LogSlotId ntLogSlotId_l = 0;
        unsigned short int profileId_l  = 0;


        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Inside initPortData of PortMapping") ;


        EqptCore::OwnEqptIdentification::getOwnShelfType(shelfType);


        if (EqptCore::OwnEqptIdentification::getOwnBoardType()==C_NRNT_A)
        {

                ntLogSlotId_l = EqptCore::OwnEqptIdentification::getOwnSlotId();

                if (EqptLib::BoardInfo::getSlotPlannedCapabilityProfile
                                (ntLogSlotId_l,profileId_l) != 
                                EqptCore::eqptNoError)
                {
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,
                                        "NT SlotPlannedCapabilityProfile not found, NETWORK assumed");
                        profileId_l = E_one_gig_remote_lt;       
                }
                else
                {
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"profileId_l %d\n", profileId_l);
                }
        }       


        PortMappingImpl::setAddressingMode(PortMapping::POSITION_BASED_ADDR_MODE) ;
        PortMappingImpl::configurationChanges_m = 0 ;
        CapabilityMgr::ErrorCode errCode = CapabilityMgr::getSystemCapability(CapabilityKeys::E_NT_CHIP_CAPABILITY_c,
                        ntChipCap);
        if (errCode != CapabilityMgr::noError)
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                "NT_CHIP_CAPABILITY not set for %X, leaving the initPortData with error") ;

        }
        else
        {

                ASAM_StructuredObjectId objectId_l(AsamIds::domainIdEqpt_c,
                                Eqpt::subsystemIdSfpMgnt_c,
                                SfpMgnt::classIdHostSfpConfigTable_c,
                                0);
                SfpMgnt::HostSfpConfigTable_ifc hostSfpConfigTable_l;
                IOCM::Status iocmStatus_l = hostSfpConfigTable_l._bind(objectId_l);
                if(iocmStatus_l != IOCM::Status_OK)
                {
                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_NON_RECOV,
                                        PRTMLabel, __FILE__, __LINE__,
                                        "IOCM bind failed: %ul, leaving the initPortData with error\n",iocmStatus_l);
                }
                else
                {
                        for ( int i = 1 ; i <= MAX_IPDPORTS ; i++)
                        {
                                Eqpt::BoardType ntioType ;
                                LanxCfgProxy proxy_lanxcfg(shelfType,  EqptCore::OwnEqptIdentification::getOwnShelfMode(), ntChipCap, profileId_l);


                                //                Sfp::HostConfigEntry            entry_l;

                                while (proxy_lanxcfg)
                                {
                                        boolean tupleFound = false ;
                                        int lanxLogPort = proxy_lanxcfg->dmGetLanxLogPort() ;
                                        if ((lanxLogPort == i) &&  PortMappingImpl::isTupleOfLanxCfgTableForMe(shelfType,
                                                                EqptCore::OwnEqptIdentification::getOwnShelfMode(),
                                                                ntChipCap,
                                                                profileId_l,
                                                                proxy_lanxcfg->dmGetShelfType(),
                                                                proxy_lanxcfg->dmGetShelfMode(),
                                                                proxy_lanxcfg->dmGetNtChipCap(),
                                                                proxy_lanxcfg->dmGetProfileType()))
                                        {
                                                int slotId = proxy_lanxcfg->dmGetLogSlot() ;
                                                unsigned short portTypeFromLanxCfgData = proxy_lanxcfg->dmGetPortType() ;

                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"1. while lanxLogPort %d portTypeFromLanxCfgData %d \n", lanxLogPort,portTypeFromLanxCfgData);

                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"1. slotId  %d dmGetSFPNo %d\n", slotId, proxy_lanxcfg->dmGetSFPNo());
                                                unsigned short portType =  PortMappingImpl::convertPorttype(ntChipCap,portTypeFromLanxCfgData, proxy_lanxcfg->dmGetSFPNo(), slotId) ;
                                                int sFPNo = proxy_lanxcfg->dmGetSFPNo();

                                                tupleFound =  TRUE ;

                                                int netPort = proxy_lanxcfg->dmGetNetPort() ;
                                                int netPortNo = netPort ;
                                                PortMappingImpl::portToIpdPortData[i].counterForThisChange = 0 ;
                                                PortMappingImpl::portToIpdPortData[i].phyPortSlot = slotId ;
                                                PortMappingImpl::portToIpdPortData[i].phyPortNbr =  netPort ;

                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"1. converted portType  %d\n", portType);

                                                if (PortMappingImpl::portToIpdPortData[i].phyPortNbr > 256)
                                                {
                                                        PortMappingImpl::portToIpdPortData[i].phyPortNbr -=256 ;
                                                }
                                                switch (portType)
                                                {
                                                        case E_PORT_MCAST :
                                                                PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_NT ;
                                                                PortMappingImpl::portToIpdPortData[i].identifier = 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_MCAST ;
                                                                PortMappingImpl::portToIpdPortData[i].phyPortType = IPD_PORT_CLI_MCAST + 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].phyPortSlot = 4353 ;
                                                                PortMappingImpl::portToIpdPortData[i].slotId = 0 ;
                                                                PortMappingImpl::portToIpdPortData[i].phyPortNbr = 1 ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                break ;
                                                        case E_PORT_IEEE1588 :
                                                                if (ntChipCap >= 3)
                                                                {
                                                                        PortMappingImpl::portToIpdPortData[i].slotId  = slotId ;
                                                                        PortMappingImpl::portToIpdPortData[i].portType =IPD_PORT_CLI_IEEE1588 ;
                                                                        PortMappingImpl::portToIpdPortData[i].boardtype = (slotId == 0x1101) ? IPD_PORT_BOARD_NTA : IPD_PORT_BOARD_NTB ;
                                                                        PortMappingImpl::portToIpdPortData[i].identifier =  1 ;
                                                                        PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                }
                                                                else
                                                                {
                                                                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                                                                        "Port type not valid for chip capability lower than 3 (=%d) for port %d while constructing the ipd port map",
                                                                                        ntChipCap, i, 0);
                                                                }
                                                                break ;

                                                                // the following is added for ALU00807075 
                                                        case E_PORT_AI :
                                                                if (ntChipCap >= 3)
                                                                {
                                                                        PortMappingImpl::portToIpdPortData[i].slotId  = slotId;
                                                                        PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_AI;
                                                                        PortMappingImpl::portToIpdPortData[i].phyPortType = IPD_PORT_CLI_AI;
                                                                        PortMappingImpl::portToIpdPortData[i].phyPortNbr = 1;
                                                                        PortMappingImpl::portToIpdPortData[i].boardtype = (slotId == 0x1101) ? IPD_PORT_BOARD_NTA : IPD_PORT_BOARD_NTB;
                                                                        PortMappingImpl::portToIpdPortData[i].identifier = sFPNo;
                                                                        PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i;
                                                                }
                                                                else
                                                                {
                                                                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                                                                        "Port type not valid for chip capability lower than 3 (=%d) for port %d while constructing the ipd port map",
                                                                                        ntChipCap, i, 0);
                                                                }
                                                                break ;
                                                                // end of addition for ALU00807075 

                                                        case E_VIRTUAL_PORT :
                                                                PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_NT ;
                                                                PortMappingImpl::portToIpdPortData[i].identifier = 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_VP ;
                                                                PortMappingImpl::portToIpdPortData[i].phyPortType = IPD_PORT_CLI_VP + 1 ;

                                                                PortMappingImpl::portToIpdPortData[i].slotId = 0 ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;

                                                                //#define TGT_PF_FTTU
#ifdef TGT_PF_FTTU
                                                                i++ ;
                                                                PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_NT ;
                                                                PortMappingImpl::portToIpdPortData[i].identifier = 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_UGW ;
                                                                PortMappingImpl::portToIpdPortData[i].slotId = 0 ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                i++ ;
                                                                PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_NTA ;
                                                                PortMappingImpl::portToIpdPortData[i].identifier = 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_CPORT ;
                                                                PortMappingImpl::portToIpdPortData[i].slotId = 0 ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                i++ ;
                                                                PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_NTB ;
                                                                PortMappingImpl::portToIpdPortData[i].identifier = 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_CPORT ;
                                                                PortMappingImpl::portToIpdPortData[i].slotId = 1 ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                i++ ;
                                                                PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_ACU ;
                                                                PortMappingImpl::portToIpdPortData[i].identifier = 1 ;
                                                                PortMappingImpl::portToIpdPortData[i].portType = IPD_PORT_CLI_100M ;
                                                                PortMappingImpl::portToIpdPortData[i].slotId = 0 ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
#endif
                                                                break ;
                                                        case E_INVALID :

                                                                break ;
                                                        case E_PHYSICAL_PORT :
                                                                {
                                                                        PortMappingImpl::portToIpdPortData[i].slotId  = slotId ;
#ifdef BOARD_ecnt_c
                                                                        netPortNo = netPort - 1;
                                                                        PortMappingImpl::portToIpdPortData[i].phyPortNbr =  netPortNo ;
#endif
                                                                        switch (proxy_lanxcfg->dmGetPhyIfType() )
                                                                        {
                                                                                case E_SFP :
                                                                                        PortMappingImpl::portToIpdPortData[i].portType =IPD_PORT_CLI_SFP ;
                                                                                        break ;
                                                                                case E_XFP :
                                                                                        PortMappingImpl::portToIpdPortData[i].portType =IPD_PORT_CLI_XFP ;
                                                                                        break ;
                                                                                case E_ETH:
                                                                                        PortMappingImpl::portToIpdPortData[i].portType =IPD_PORT_CLI_ETH ;
                                                                                        break ;
                                                                                case E_CFP:
                                                                                        PortMappingImpl::portToIpdPortData[i].portType =IPD_PORT_CLI_CFP ;
                                                                                        break ;
                                                                        }
                                                                        if (netPort == 0)
                                                                        {
                                                                                netPort  = sFPNo ;
                                                                                netPortNo = sFPNo ;
                                                                        }
                                                                        PortMappingImpl::portToIpdPortData[i].phyPortType = PortMappingImpl::portToIpdPortData[i].portType + 1 ;


                                                                        PortMappingImpl::portToIpdPortData[i].boardtype = (slotId == 0x1101) ? IPD_PORT_BOARD_NTA : IPD_PORT_BOARD_NTB ;
                                                                        PortMappingImpl::portToIpdPortData[i].identifier =  netPort ;
                                                                        PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                        if (proxy_lanxcfg->dmGetNtChipCap() == 1) // The NT can have downlinks.
                                                                        {
                                                                                PortMappingImpl::populateCableLsmData(i,
                                                                                                netPortNo,
                                                                                                PortMappingImpl::portToIpdPortData[i].portType ,
                                                                                                netPort,
                                                                                                &hostSfpConfigTable_l,
                                                                                                PortMappingImpl::portToIpdPortData[i].
                                                                                                boardtype,
                                                                                                slotId) ;

                                                                        }

                                                                        break ;
                                                                }
                                                        case E_PORT_LT_LAG :
                                                                {
                                                                        PortMappingImpl::portToIpdPortData[i].identifier = 0 ;
                                                                        PortMappingImpl::portToIpdPortData[i].boardtype = IPD_PORT_BOARD_LT ;
                                                                        PortMappingImpl::portToIpdPortData[i].slotId  = slotId ;
                                                                        PortMappingImpl::ipdPortDataToPort[ PortMappingImpl::hashPortData( PortMappingImpl::portToIpdPortData[i])] = i ;
                                                                        break ;
                                                                }

                                                        case E_PORT_IO_LAG :
                                                                {


                                                                        if (EqptLib::BoardInfo::getPlannedBoardType(slotId, ntioType) == EqptCore::eqptNoError)
                                                                        {
                                                                                //ANTmt54543
                                                                                Eqpt::ProfileId profileId_l = 0;  //Sfp::not_applicable_c
                                                                                EqptLib::BoardInfo::getProfileId(slotId, profileId_l);
                                                                                //ANTmt54543
                                                                                SfpPortMapProxy sfpPortMappingProxy(shelfType,ntioType, profileId_l, sFPNo) ;
                                                                                if (sfpPortMappingProxy &&  PortMappingImpl::isTupleOfSfpPortMappingTableForMe
                                                                                                (shelfType, ntioType, profileId_l, sFPNo,
                                                                                                 sfpPortMappingProxy->dmGetShelfType(),
                                                                                                 sfpPortMappingProxy->dmGetBoardType(),
                                                                                                 sfpPortMappingProxy->dmGetProfileId(),
                                                                                                 sfpPortMappingProxy->dmGetSFPNo()))
                                                                                {
                                                                                        unsigned short hostSfpConfigEntry_l = sfpPortMappingProxy->dmGetFacePlateNo();
                                                                                        if (sfpPortMappingProxy->dmGetPhyIfType() == E_XFP || (sfpPortMappingProxy->dmGetPhyIfType() == E_SFP && profileId_l == Sfp::single_active_c))
                                                                                        {
                                                                                                hostSfpConfigEntry_l = hostSfpConfigEntry_l + 256;
                                                                                        }
                                                                                        PortMapping::IpdPortCliType  phyType  =    (sfpPortMappingProxy->dmGetPhyIfType() == E_XFP) ? IPD_PORT_CLI_XFP : IPD_PORT_CLI_SFP ;
                                                                                        PortMappingImpl::populateCableLsmData(i,
                                                                                                        hostSfpConfigEntry_l,
                                                                                                        phyType,
                                                                                                        sfpPortMappingProxy->dmGetFacePlateNo(),
                                                                                                        &hostSfpConfigTable_l,
                                                                                                        IPD_PORT_BOARD_NTIO1,
                                                                                                        0) ;
                                                                                }
                                                                        }

                                                                        break ;
                                                                        case E_PORT_ACU_LAG :
                                                                        case E_PORT_EHiGiG :
                                                                        case E_PORT_EHiGiG1 :
                                                                        case E_PORT_CONTROL :

                                                                        break ;
                                                                        default :
                                                                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                                                                        "Port type unknown %d for port %d while constructing the ipd port map",
                                                                                        portType, i, 0);
                                                                        break ;

                                                                }

                                                }
                                        }
                                        if (tupleFound == TRUE)
                                        {
                                                proxy_lanxcfg++ ;


                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Dual Port\n" );
                                                if (proxy_lanxcfg)
                                                {
                                                        int llport =  proxy_lanxcfg->dmGetLanxLogPort() ;

                                                        if ((llport == lanxLogPort)&&  PortMappingImpl::isTupleOfLanxCfgTableForMe(shelfType,
                                                                                EqptCore::OwnEqptIdentification::getOwnShelfMode(),
                                                                                ntChipCap,
                                                                                profileId_l,
                                                                                proxy_lanxcfg->dmGetShelfType(),
                                                                                proxy_lanxcfg->dmGetShelfMode(),
                                                                                proxy_lanxcfg->dmGetNtChipCap(),
                                                                                proxy_lanxcfg->dmGetProfileType()))

                                                        {
                                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Dual Port is present\n" );

                                                                // Handle Dual port
                                                                PortMappingImpl::IPDPORTDATA extraPortData ;
                                                                extraPortData =  PortMappingImpl::portToIpdPortData[i] ;
                                                                extraPortData.identifier = proxy_lanxcfg->dmGetSFPNo();
                                                                extraPortData.portType = (proxy_lanxcfg->dmGetPhyIfType() == E_XFP) ? IPD_PORT_CLI_XFP : IPD_PORT_CLI_SFP ;
                                                                PortMappingImpl::ipdPortDataToPort[ PortMappingImpl:: PortMappingImpl::hashPortData(extraPortData)] = i ;
                                                        }
                                                        break ;
                                                }
                                        }
                                        else
                                        {
                                                proxy_lanxcfg++ ;
                                        }
                                }
                        }
                        PortMappingImpl::portDataIsInitialised = TRUE ;
                }
        }
}
boolean PortMapping::isInitialised()

{
        return  PortMappingImpl::portDataIsInitialised ;
}


tPortId PortMapping::logPort_to_tPortId(unsigned int portId)

{
        tPortId returnValue  =  PortMappingImpl::makeTmnxPortId( portId) ;

        return returnValue ;
}


unsigned int  PortMapping::tPortId_to_logPort(tPortId port)

{
        unsigned int returnValue  =  0 ;
        if ((port & 0xFFE07FFF) == 0x02200000)
        {
                returnValue = PortMappingImpl::makePortIdFromTmnxPortId(port) ;
        }
        else
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,
                                "Not valid port id presented 0x%X to tPortId_to_logPort ",port, 0);
        }
        return returnValue ;
}



tInt32 PortMapping::configRemoteLtPort(tPortId ipd_port, Eqpt::LogSlotId slotId)

{
        tInt32 returnValue = E_FAILURE ;
        unsigned int ipdport =  InfraMgntNt::PortMappingImpl::makePortIdFromTmnxPortId(ipd_port) ;
        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"configRemoteLtPort with ipdport = %d and slotid = %d.%s",ipdport, slotId, LINEFEED) ;

        //ALU01748176  (on NFXS-B with NANT-A) 
        //     New Err recovery, While assigning the IONT port to remot-lt 
        //       (remoteLtPort was presented a not-NTIO port)
        //Error is only there for NANT-A
        //In case the NTIO is planned and inserted after startup,
        //then the SlotId 0 is not present in the map remoteLTs_m,
        //and this gives then the error when configuring the remote lt.
        //When at least one reset is done then the SlotId 0 is present in the map,
        //because then the NTIO is present ....
        //trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Search for slotid 0 in remoteLTs_m");
        if (PortMappingImpl::remoteLTs_m.find(0) ==  PortMappingImpl::remoteLTs_m.end())
        {
                //trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Slotid 0 in remoteLTs_m not present");
                if (EqptCore::OwnEqptIdentification::getOwnBoardType() == C_NANT_A)
                {
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"configRemoteLtPort for NANT_A. Add slotid 0 to remoteLTs_m ...");
                        PortMappingImpl::remoteLTs_m[0] = true ;
                }
        }

        if (PortMappingImpl::portToIpdPortData.find(ipdport) != PortMappingImpl::portToIpdPortData.end())
        {
                PortMappingImpl::IPDPORTDATA portData = PortMappingImpl::portToIpdPortData[ipdport] ;
                bool isARemoteLTAlready = false ;

                if (PortMappingImpl::remoteLTs_m.find(portData.slotId) !=  PortMappingImpl::remoteLTs_m.end())
                {
                        isARemoteLTAlready = true ;
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"configRemoteLtPort with ipdport = %d has already a remote LT with slotid = %d.%s",ipdport, portData.slotId, LINEFEED) ;
                };
                if ((PortMappingImpl::portToIpdPortData[ipdport].boardtype == IPD_PORT_BOARD_NTIO1) || (isARemoteLTAlready)
                                || (PortMappingImpl::portToIpdPortData[ipdport].boardtype == IPD_PORT_BOARD_NTA) ||
                                (PortMappingImpl::portToIpdPortData[ipdport].boardtype == IPD_PORT_BOARD_NTB))

                {

                        int index =  PortMappingImpl::hashPortData(portData) ;
                        if ((isARemoteLTAlready) && (portData.slotId != 0))
                        {
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"configRemoteLtPort, removing remote LT entry  %d .%s",portData.slotId, LINEFEED) ;
                                PortMappingImpl::remoteLTs_m.erase(portData.slotId) ;
                        }

                        int phyPortNbr = 0 ;
                        if (portData.phyPortNbr != 0)
                        {
                                phyPortNbr = portData.phyPortNbr ;
                        }
                        else
                        {
                                phyPortNbr = portData.identifier ;
                        }
                        PortMappingImpl::ipdPortDataToPort.erase(index) ; // Remove old data
                        portData.slotId = slotId ;
                        portData.phyPortNbr = phyPortNbr ;
                        portData.boardtype = IPD_PORT_BOARD_LT ;
                        portData.identifier = 0 ;
                        portData.portType = (InfraMgntNt::PortMapping::IpdPortCliType)0 ;
                        index = PortMappingImpl::hashPortData(portData) ;
                        PortMappingImpl::ipdPortDataToPort[index] = ipdport ;

                        PortMappingImpl::remoteLTs_m[slotId] = true ;
                        PortMappingImpl::portToIpdPortData[ipdport].slotId = slotId ;
                        PortMappingImpl::portToIpdPortData[ipdport].boardtype =  IPD_PORT_BOARD_LT  ;
                        PortMappingImpl::portToIpdPortData[ipdport].identifier = 0 ;
                        PortMappingImpl::portToIpdPortData[ipdport].phyPortNbr = phyPortNbr ;
                        PortMappingImpl::portToIpdPortData[ipdport].portType = (InfraMgntNt::PortMapping::IpdPortCliType)0 ;
                        returnValue = E_SUCCESS ;
                        PortMappingImpl::configurationChanges_m++ ;
                        PortMappingImpl::portToIpdPortData[ipdport].counterForThisChange =   PortMappingImpl::configurationChanges_m ;
                        generateTrap_EqptPortMappingChanges_ChangeOccured(PortMappingImpl::configurationChanges_m) ;
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"configRemoteLtPort with ipdport = %d and slotid = %d.completed.%s",ipdport, slotId, LINEFEED) ;

                }
                else
                {
                        err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,"configRemoteLtPort was presented a not-NTIO port") ;
                }
        }
        else
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, PRTMLabel, __FILE__, __LINE__,"configRemoteLtPort encountered an unknown port id", LINEFEED) ;
        }



        return returnValue ;

}

tInt32 PortMapping::getPortMapping(tUint32 ipd_port, tUint32 *phyPortSlot, tUint32 *facePlateNum, tUint32 *phyItfType, Eqpt::LogSlotId *lsmSlotId,tUint32 *logPortType)
{
        tInt32 returnValue = E_FAILURE ;
        int slotId = 0 ;
        unsigned short sFPNo = 0 ;
        //    unsigned short netPort = 0;
        int lanxLogPort = -1 ;
        unsigned int ntChipCap = 1;
        unsigned short int profileId  = 0;
        Eqpt::LogSlotId ntLogSlotId_l = 0;


        Eqpt::ShelfType shelfType = 0 ;
        unsigned short portType = E_INVALID ;
        boolean needToTraverseTables = true ;
        //    Sfp::HostConfigEntry            entry_l;

        bool converted = TaskConversion::TaskConvert::convertTask2IOCM();
        assert_r (converted);

        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Inside getPortMapping Lanx Port %d\n", ipd_port);

        if(PortMappingImpl::portToIpdPortData.find(ipd_port) != PortMappingImpl::portToIpdPortData.end())
        {
                *phyPortSlot = PortMappingImpl::portToIpdPortData[ipd_port].phyPortSlot ;
                *phyItfType = PortMappingImpl::portToIpdPortData[ipd_port].phyPortType ;
                if (*phyItfType == 0)
                {
                        *phyItfType = 1 ;
                }
                *facePlateNum = PortMappingImpl::portToIpdPortData[ipd_port].phyPortNbr ;
        }
        else
        {
                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"PortData NOT FOUND for ipdPort=%d \r\n", ipd_port);
                return returnValue;
        }

        CapabilityMgr::ErrorCode errCode = CapabilityMgr::getSystemCapability(CapabilityKeys::E_NT_CHIP_CAPABILITY_c,
                        ntChipCap);
        if (errCode != CapabilityMgr::noError)
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_RECOV, "IPDMT", __FILE__, __LINE__,
                                "NT_CHIP_CAPABILITY not set for %X, defaulting to 1 [%X]",
                                EqptCore::OwnEqptIdentification::getOwnBoardType(), errCode);
                ntChipCap = 1;
        }

        ASAM_StructuredObjectId objectId_l(AsamIds::domainIdEqpt_c,
                        Eqpt::subsystemIdSfpMgnt_c,
                        SfpMgnt::classIdHostSfpConfigTable_c,
                        0);
        SfpMgnt::HostSfpConfigTable_ifc hostSfpConfigTable_l;
        IOCM::Status iocmStatus_l = hostSfpConfigTable_l._bind(objectId_l);

        if(iocmStatus_l != IOCM::Status_OK)
        {
                err_printf(ERR_COM_UNEXP_COND, ERR_CLASS_NON_RECOV,
                                "IPDMT", __FILE__, __LINE__,
                                "IOCM bind failed: %ul\n",iocmStatus_l);
        }


        EqptCore::OwnEqptIdentification::getOwnShelfType(shelfType);
        if (EqptCore::OwnEqptIdentification::getOwnBoardType()==C_NRNT_A)
        {
                ntLogSlotId_l = EqptCore::OwnEqptIdentification::getOwnSlotId();
                if (EqptLib::BoardInfo::getSlotPlannedCapabilityProfile(ntLogSlotId_l,profileId) != EqptCore::eqptNoError)
                {

                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"getting profileId %d failed \n", profileId);
                        profileId = E_one_gig_remote_lt;
                }
                else
                {
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"profileId %d\n", profileId);

                }       
        }//if ((shelfType == E_NFXR_A))

        LanxCfgProxy proxy_lanxcfg(shelfType,  EqptCore::OwnEqptIdentification::getOwnShelfMode(), ntChipCap, profileId);

        while ((proxy_lanxcfg) && (needToTraverseTables))
        {
                lanxLogPort = proxy_lanxcfg->dmGetLanxLogPort() ;

                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"while lanxLogPort  %d\n", lanxLogPort);
                if ((lanxLogPort == (int)ipd_port) && PortMappingImpl::isTupleOfLanxCfgTableForMe(shelfType,
                                        EqptCore::OwnEqptIdentification::getOwnShelfMode(),
                                        ntChipCap,
                                        profileId,
                                        proxy_lanxcfg->dmGetShelfType(),
                                        proxy_lanxcfg->dmGetShelfMode(),
                                        proxy_lanxcfg->dmGetNtChipCap(),
                                        proxy_lanxcfg->dmGetProfileType()))
                {
                        slotId = proxy_lanxcfg->dmGetLogSlot() ;
                        unsigned short portTypeFromLanxCfgData = proxy_lanxcfg->dmGetPortType() ;
                        portType = PortMappingImpl::convertPorttype(ntChipCap,  portTypeFromLanxCfgData, proxy_lanxcfg->dmGetSFPNo(), slotId);

                        sFPNo = proxy_lanxcfg->dmGetSFPNo();
                        //              netPort = proxy_lanxcfg->dmGetNetPort() ;


                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"slotId=%d ipdPort=%d portType=%d\r\n", slotId, ipd_port,portType) ;
                        needToTraverseTables =
                                !((portType == E_VIRTUAL_PORT) ||
                                                (portType == E_INVALID) ||
                                                //(portType ==  E_PORT_EHiGiG) || (portType ==  E_PORT_IHiGiG) ||
                                                (portType == E_PORT_CONTROL ));

                }
                else
                {
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"go for Next lanxlogPort %d\n", lanxLogPort) ;
                        lanxLogPort = -1;
                }
                proxy_lanxcfg++;
                if (lanxLogPort  != -1)
                {
                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m," break logPort %d", lanxLogPort) ;
                        break;
                }
        }
        if ((lanxLogPort == (int)ipd_port)  || (!needToTraverseTables))
        {

                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"IPD PortType=%d ipdPort=%d\r\n", portType, ipd_port) ;
                switch (portType)
                {
                        case E_VIRTUAL_PORT:
                                *logPortType = E_EqptPortMappingLogPortType_internal ;
                                *lsmSlotId = E_UNKNOWN;
                                *phyPortSlot = E_UNKNOWN ;
                                *phyItfType = E_EqptPortMappingPhyPortType_vp ;
                                *facePlateNum = 1 ;
                                returnValue = E_SUCCESS;
                                break ;
                        case E_INVALID:
                                returnValue = E_FAILURE;
                                break;
                        case E_PORT_MCAST:
                                // the following is added for ALU00807075 
                        case E_PORT_AI :
                                // end of addition for ALU00807075 
                                *logPortType = E_EqptPortMappingLogPortType_uplink; //uplink (NETWORK_PORT_TYPE or UNUSED_PORT_TYPE)
                                *lsmSlotId = E_UNKNOWN;
                                returnValue = E_SUCCESS;

                                break;
                        case E_PORT_IEEE1588:
                                *logPortType = E_EqptPortMappingLogPortType_internal ;
                                *lsmSlotId = E_UNKNOWN;
                                //           *phyPortSlot = E_UNKNOWN ;
                                *phyItfType = E_EqptPortMappingPhyPortType_ieee ;
                                *facePlateNum = 1 ;
                                returnValue = E_SUCCESS;
                                break;
                                // the following is added for ALU00807075 
                        case E_PHYSICAL_PORT:
                                {
                                        *logPortType = E_EqptPortMappingLogPortType_uplink; //uplink (NETWORK_PORT_TYPE or UNUSED_PORT_TYPE)
                                        *lsmSlotId = E_UNKNOWN;
                                        Eqpt::BoardType boardType = EqptCore::OwnEqptIdentification::getOwnBoardType() ;
                                        if ((proxy_lanxcfg->dmGetNtChipCap() == 1)
                                                        && (boardType != C_NANT_A))// The mini NT & agnt-a can have downlinks.
                                        {
                                                PortMappingImpl::getCableLsmData(ipd_port, sFPNo, logPortType, lsmSlotId, &hostSfpConfigTable_l) ;

                                        }


                                        returnValue = E_SUCCESS;

                                        break;
                                }
                        case E_PORT_LT_LAG:
                                *logPortType = E_EqptPortMappingLogPortType_hostlink;
                                *lsmSlotId = PortMappingImpl::portToIpdPortData[ipd_port].slotId;
                                *phyPortSlot = E_UNKNOWN;
                                *facePlateNum = 0xFF;
                                returnValue = E_SUCCESS;
                                break;
                        case E_PORT_IO_LAG:
                                returnValue = E_FAILURE;
                                Eqpt::BoardType ntioType ;

                                *phyPortSlot = slotId;
                                do
                                {
                                        Eqpt::ProfileId profileId_l = 0;  //Sfp::not_applicable_c
                                        if(EqptLib::BoardInfo::getPlannedBoardType(slotId, ntioType) != EqptCore::eqptNoError)
                                                break;

                                        unsigned long rc = EqptLib::BoardInfo::getProfileId(slotId, profileId_l);
                                        if (rc != EqptCore::eqptNoError)
                                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"EqptLib::BoardInfo::getProfileId for slotID %x fails with rc=%d \r\n", slotId, rc) ;

                                        SfpPortMapProxy sfpPortMappingProxy(shelfType,ntioType,profileId_l,sFPNo) ;
                                        trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"%d %d %d %d %d CfgDXm %d %d %d %d\r\n",ipd_port,shelfType,ntioType,profileId_l,sFPNo,sfpPortMappingProxy->dmGetShelfType(),sfpPortMappingProxy->dmGetBoardType(),sfpPortMappingProxy->dmGetProfileId(),sfpPortMappingProxy->dmGetSFPNo()) ;
                                        if (sfpPortMappingProxy && PortMappingImpl::isTupleOfSfpPortMappingTableForMe(shelfType, ntioType, profileId_l, sFPNo,
                                                                sfpPortMappingProxy->dmGetShelfType(),
                                                                sfpPortMappingProxy->dmGetBoardType(),
                                                                sfpPortMappingProxy->dmGetProfileId(),
                                                                sfpPortMappingProxy->dmGetSFPNo()))
                                        {

                                                unsigned short hostSfpConfigEntry_l = sfpPortMappingProxy->dmGetFacePlateNo();
                                                if (sfpPortMappingProxy->dmGetPhyIfType() == E_XFP || (sfpPortMappingProxy->dmGetPhyIfType() == E_SFP && profileId_l == Sfp::single_active_c))
                                                {
                                                        hostSfpConfigEntry_l = hostSfpConfigEntry_l + 256;
                                                }
                                                *facePlateNum = sfpPortMappingProxy->dmGetFacePlateNo();
                                                PortMappingImpl::getCableLsmData(ipd_port,hostSfpConfigEntry_l, logPortType, lsmSlotId, &hostSfpConfigTable_l) ;

                                        }
                                        else
                                                break;

                                        returnValue = E_SUCCESS;
                                }while(false);

                                break;
                        case E_PORT_ACU_LAG:
                                //case E_PORT_EHiGiG:
                                //case E_PORT_IHiGiG:
                        case E_PORT_CONTROL:
                                returnValue = E_FAILURE;
                                break;
                        default:
                                trc_printf(InfraMgntNt::PortMappingImpl::traceId_m,"Port type unknown %d in generating the <IPD PORT> part \r\n",ipd_port) ;
                                returnValue = E_FAILURE;
                                break;
                }
        }
        return returnValue;
}


tPortId PortMappingImpl::makeTmnxPortId(unsigned int portId)

{
        return (0x02200000 + (portId << 15));
}
unsigned int  PortMappingImpl::makePortIdFromTmnxPortId(tPortId tmnxPortId)

{
        return (tmnxPortId - 0x02200000) >> 15 ;
}

bool PortMappingImpl::getVal(char * paramStr, int * idx, unsigned long *num)

{
        unsigned int val;
        bool returnValue = false ;
        int index = *idx ;

        returnValue = !dbg_scanint(paramStr,&index,&val)  ;

        if (returnValue)
        {
                *num=val;
                if (val >=2208000)
                { // Propably encoded number
                        returnValue = !dbg_scanhex (paramStr,idx,&val) ;
                        if (returnValue)
                        {
                                *num=val;
                        }
                }
                else
                {
                        *idx = index ;
                }
        }

        return returnValue ;
}

unsigned int PortMapping::getConfigurationChanges()
{
        return PortMappingImpl::configurationChanges_m ;
}

unsigned int PortMapping::getConfigurationChangesForPort(unsigned int portId)


{
        unsigned int returnValue = 0 ;
        if(PortMappingImpl::portToIpdPortData.find(portId) != PortMappingImpl::portToIpdPortData.end())
        {

                returnValue = InfraMgntNt::PortMappingImpl::portToIpdPortData[portId].counterForThisChange ;
        }

        return returnValue ;
}

void PortMappingImpl::showHelp()
{
        dbg_printf("PortMapping :\n\r");
        dbg_printf("-------------------\n\r\n\r");
        dbg_printf("16 commands are supported :\n\r") ;
        dbg_printf("port2identifier <portid>, say 1 (or its encoded equivalent 2208000)  e.g. For a lag id an input 50000001 e.g. is expected.)\n\r") ;
        dbg_printf("identifier2port <board> <rack> <shelf> <slot> porttype identifiere.g.\n\r") ;
        dbg_printf("<board> -> \t\tenum{IPD_PORT_BOARD_NT=1, \n\r") ;
        dbg_printf("\t\tIPD_PORT_BOARD_NTA, IPD_PORT_BOARD_NTB, IPD_PORT_BOARD_LT, IPD_PORT_BOARD_NTIO1, IPD_PORT_BOARD_NTIO2,IPD_PORT_BOARD_ACU, IPD_PORT_INVALID_BOARD }\n\r") ;
        dbg_printf("<port> -> enum { IPD_PORT_CLI_SFP =1, IPD_PORT_CLI_XFP, IPD_PORT_CLI_MGNT, IPD_PORT_CLI_VP, IPD_PORT_CLI_UGW, IPD_PORT_CLI_CPORT, IPD_PORT_CLI_100M,  IPD_PORT_CLI_LAG, IPD_PORT_CLI_ETH, IPD_PORT_CLI_MCAST, IPD_PORT_CLI_IEEE1588, IPD_PORT_CLI_AI, IPD_PORT_CLI_INVALID }\n\r") ;
        dbg_printf("configRemoteLtPort <ipdport> <slotId>\n\r") ;
        dbg_printf("lp2tp <logport>\n\r") ;
        dbg_printf("tp2lp <ipdport>\n\r") ;
        dbg_printf("gadm (get addressing mode)\n\r") ;
        dbg_printf("sadm (set addressing mode) <addrmode> (should be 2 = pos based or 3 = type based)\n\r") ;
        dbg_printf("dump_portMapping \n\r");
        dbg_printf("sc (show configuration change counter) \n\r") ;
        dbg_printf("getntxportid <slotid>  <porttype> <identifier> \n\r") ;
        dbg_printf("getltportid <slotid>\n\r") ;
        dbg_printf("getfpp ipdportnbr\n\r") ;
        dbg_printf("getportid4downlink <slotid> <faceplatenbr> <porttype>\n\r") ;
}
void PortMappingImpl::printDebugOutput(char *ft, int rc, int port)

{
        dbg_printf("%s, identifier2port returns %s, port = %d.%s",ft,(rc == PortMapping::E_SUCCESS) ? "SUCCESS" : "FAILURE", port, LINEFEED) ;
}

unsigned long PortMappingImpl::procCmd(char* str_i, int* idxPtr_i, char* cmdStr_i)
{
        if (strcmp(cmdStr_i, "goodntvp") ==0)
        {
                tPortId ipd_port ;

                int rc =  PortMapping::identifier2port(&ipd_port,
                                InfraMgntNt::PortMapping::IPD_PORT_BOARD_NT ,
                                0,
                                0,
                                0,
                                InfraMgntNt::PortMapping::IPD_PORT_CLI_VP,
                                1,
                                getAddressingMode() ,
                                0) ;
                printDebugOutput("For the virtual port",rc, makePortIdFromTmnxPortId(ipd_port)) ;
        }
        else if (strcmp(cmdStr_i, "goodnt-acport") == 0)
        {
                tPortId ipd_port ;
                int rc = PortMapping::identifier2port(&ipd_port,
                                InfraMgntNt::PortMapping::IPD_PORT_BOARD_NTA ,
                                0,
                                0,
                                0,
                                InfraMgntNt::PortMapping::IPD_PORT_CLI_CPORT,
                                1,
                                getAddressingMode() ,
                                0) ;
                printDebugOutput("For the cport, nt-a side,", rc, makePortIdFromTmnxPortId(ipd_port)) ;
        }

        else if (strcmp(cmdStr_i, "goodnt-a") == 0)
        {
                tPortId ipd_port ;
                int rc = PortMapping::identifier2port(&ipd_port,
                                InfraMgntNt::PortMapping::IPD_PORT_BOARD_NTA ,
                                0,
                                0,
                                0,
                                InfraMgntNt::PortMapping::IPD_PORT_CLI_SFP,
                                1,
                                getAddressingMode() ,
                                0) ;
                printDebugOutput("For the nt-a side", rc, makePortIdFromTmnxPortId(ipd_port)) ;
        }

        else if (strcmp(cmdStr_i, "port2identifier") == 0)
        {
                boolean parsingOk = true ;
                unsigned long  value = 0 ;
                int * idx = idxPtr_i ;
                if (parsingOk == getVal(str_i, idx, &value))
                {
                        InfraMgntNt::PortMapping::IpdPortBoardType board ;
                        tUint32 rack, shelf,slot, identifier ;
                        InfraMgntNt::PortMapping::IpdPortCliType porttype ;
                        tPortId tportid = 0 ;
                        bool isHexValue = false ;
                        if (((value & 0xFFE07FFF) == 0x02200000) ||  /* check if the port is an encoded number */
                                        ((value & 0xFF000000) == 0x50000000)) /* lag id */

                        {
                                tportid = value ;
                                isHexValue = true ;
                        }
                        else
                        {
                                tportid = makeTmnxPortId(value) ;
                        }
                        if ( PortMapping::port2Identifier(tportid, &board,&rack,&shelf,&slot,&porttype,&identifier, PortMappingImpl::getAddressingMode()) == PortMapping::E_SUCCESS)
                        {
                                if (isHexValue)
                                {

                                        dbg_printf("port2identifier succeeded for port %X hex board = %d, rack = %d, shelf = %d, slot = %d, porttype = %d, identifier = %d %s", value, board, rack, shelf, slot, porttype, identifier,LINEFEED) ;
                                }
                                else
                                {

                                        dbg_printf("port2identifier succeeded for port %d board = %d, rack = %d, shelf = %d, slot = %d, porttype = %d, identifier = %d %s", value, board, rack, shelf, slot, porttype, identifier,LINEFEED) ;
                                }
                                dbg_printf("The physical port number stored in the map is = %d %s", portToIpdPortData[value].phyPortNbr,LINEFEED) ;

                        }
                        else
                        {
                                if (isHexValue)
                                {
                                        dbg_printf("port2identifier failed for port %X hex %s", value, LINEFEED) ;
                                }
                                else
                                {
                                        dbg_printf("port2identifier failed for port %d  %s", value, LINEFEED) ;
                                }
                        }
                }
        }

        else if (strcmp(cmdStr_i, "identifier2port") == 0)

        {
                InfraMgntNt::PortMapping::IpdPortBoardType board = InfraMgntNt::PortMapping::IPD_PORT_BOARD_NT ;
                tUint32 rack = 0;
                tUint32 shelf = 0 ;
                tUint32 slot = 0 ;
                InfraMgntNt::PortMapping::IpdPortCliType porttype = InfraMgntNt::PortMapping::IPD_PORT_CLI_INVALID ;
                tUint32 identifier = 0 ;
                unsigned long value = 0 ;
                bool parsingOk = true ;
                int * idx = idxPtr_i ;
                if (parsingOk == getVal(str_i, idx, &value))
                {
                        board = (InfraMgntNt::PortMapping::IpdPortBoardType)value ;
                }

                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        rack = value ;
                }

                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        shelf = value ;
                }

                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        slot = value ;
                }


                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        porttype = (InfraMgntNt::PortMapping::IpdPortCliType)value ;
                }
                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        identifier = value ;
                }
                if (parsingOk)
                {
                        tPortId  port_ipd ;
                        if ( PortMapping::identifier2port(&port_ipd, board, rack, shelf, slot, porttype, identifier, getAddressingMode() , 0) ==  PortMapping::E_SUCCESS)
                        {

                                if ((porttype == PortMapping::IPD_PORT_CLI_LAG) && (board == PortMapping::IPD_PORT_INVALID_BOARD))
                                {
                                        dbg_printf("identifier2port returns fine with lag id =  0x%X %s",port_ipd, LINEFEED) ;
                                }
                                else
                                {
                                        dbg_printf("identifier2port returns fine with port id = %d%s",makePortIdFromTmnxPortId(port_ipd), LINEFEED) ;
                                }
                        }
                        else
                        {
                                dbg_printf("identifier2port returns failure!%s", LINEFEED) ;
                        }

                }
        }

        else if (strcmp(cmdStr_i, "configRemoteLtPort") == 0)

        {
                tPortId ipdport = 0 ;
                tUint32 slotid = 0 ;
                int * idx = idxPtr_i ;
                bool parsingOk = true ;
                unsigned long value = 0 ;

                if (parsingOk == getVal(str_i, idx, &value))
                {
                        ipdport = InfraMgntNt::PortMappingImpl::makeTmnxPortId(value) ;

                }

                if ((parsingOk) && (dbg_scanhex(str_i, idx, &slotid)))
                {
                        parsingOk = false ;
                }

                if (parsingOk)
                {
                        if ( PortMapping::configRemoteLtPort(ipdport, slotid) ==  PortMapping::E_SUCCESS)
                        {
                                dbg_printf("configRemoteLtPort returned fine %s", LINEFEED) ;
                        }
                        else
                        {
                                dbg_printf("configRemoteLtPort returns failure!%s", LINEFEED) ;
                        }
                }
                else
                {
                        dbg_printf("configRemoteLtPort ignored, parsing error.%s", LINEFEED) ;

                }
        }

        else if (strcmp(cmdStr_i, "tp2lp") == 0)

        {
                tPortId ipdport = 0 ;
                int * idx = idxPtr_i ;
                bool parsingOk = true ;
                unsigned long value = 0 ;

                if (parsingOk == getVal(str_i, idx, &value))
                {
                        ipdport = value ;
                }

                if (parsingOk)
                {
                        dbg_printf("tPortId_to_logPort returned %d %s", PortMapping::tPortId_to_logPort(ipdport), LINEFEED) ;
                }
        }
        else if (strcmp(cmdStr_i, "lp2tp") == 0)

        {
                unsigned int logPort = 0 ;
                int * idx = idxPtr_i ;
                bool parsingOk = true ;
                unsigned long value = 0 ;

                if (parsingOk == getVal(str_i, idx, &value))
                {
                        logPort = value ;
                }

                if (parsingOk)
                {
                        dbg_printf("logPort_to_tPortId returned 0x%X %s", PortMapping::logPort_to_tPortId(logPort), LINEFEED) ;
                }
        }
        else if (strcmp(cmdStr_i, "gadm") == 0)

        {

                PortMapping::SlotAddrMode value = getAddressingMode() ;

                if ((value == PortMapping::POSITION_BASED_ADDR_MODE) || (value == PortMapping::TYPE_BASED_ADDR_MODE))
                {
                        if (value ==  PortMapping::POSITION_BASED_ADDR_MODE)
                        {
                                dbg_printf("The position based addressing mode is active.%s",  LINEFEED) ;
                        }
                        else
                        {
                                dbg_printf("The type based addressing mode is active.%s",  LINEFEED) ;
                        }

                }
        }
        else if (strcmp(cmdStr_i, "sadm") == 0)

        {
                unsigned long value = 0 ;
                bool parsingOk = true ;
                int * idx = idxPtr_i ;

                if (parsingOk == getVal(str_i, idx, &value))
                {
                        if ((value ==  PortMapping::POSITION_BASED_ADDR_MODE) || (value ==  PortMapping::TYPE_BASED_ADDR_MODE))
                        {
                                setAddressingMode((PortMapping::SlotAddrMode)value) ;
                                if (getAddressingMode() ==  PortMapping::POSITION_BASED_ADDR_MODE)
                                {
                                        dbg_printf("The position based addressing mode is active.%s",  LINEFEED) ;
                                }
                                else
                                {
                                        dbg_printf("The type based addressing mode is active.%s",  LINEFEED) ;
                                }

                        }
                }


        }
        else if (strcmp(cmdStr_i, "sc") == 0)
        {
                //              unsigned long value = 0 ;
                //              bool parsingOk = true ;
                //              int * idx = idxPtr_i ;

                dbg_printf("Since initiation, the port mapping configuration changed %d times..%s",PortMapping::getConfigurationChanges(), LINEFEED) ;
        }
        else    if (strcmp(cmdStr_i, "getntxportid") == 0)

        {
                InfraMgntNt::PortMapping::IpdPortCliType porttype = InfraMgntNt::PortMapping::IPD_PORT_CLI_INVALID ;
                tUint32 identifier = 0 ;
                tUint32 slotid = 0 ;
                unsigned int valueint = 0 ;
                unsigned long value = 0 ;
                bool parsingOk = true ;
                int * idx = idxPtr_i ;

                if ((parsingOk) && (parsingOk == !dbg_scanhex(str_i, idx, &valueint))) // if dbg_scanhex is ok, it returns 0!
                {
                        slotid = valueint ;
                }
                else
                {
                        parsingOk = false ;
                }

                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        porttype = (InfraMgntNt::PortMapping::IpdPortCliType)value ;
                }
                else
                {
                        parsingOk = false ;
                }
                if ((parsingOk) && (parsingOk == getVal(str_i, idx, &value)))
                {
                        identifier = value ;
                }
                else
                {
                        parsingOk = false ;
                }
                if (parsingOk)
                {
                        int  port_ipd ;
                        if ( PortMapping::getNTxPortId(port_ipd, slotid, porttype, identifier) ==  PortMapping::E_SUCCESS)
                        {


                                dbg_printf("getntxportidreturns fine with port id =  %d %s",port_ipd, LINEFEED) ;

                        }
                        else
                        {
                                dbg_printf("getntxportid returns failure!%s", LINEFEED) ;
                        }

                }
                else
                {
                        dbg_printf("Parsing failed for getntxportid!%s", LINEFEED) ;
                }
        }
        else    if (strcmp(cmdStr_i, "getltportid") == 0)
        {
                tUint32 slotid = 0 ;
                unsigned int valueint = 0 ;
                bool parsingOk = true ;
                int * idx = idxPtr_i ;

                if (parsingOk == !dbg_scanhex(str_i, idx, &valueint)) // if dbg_scanhex is ok, it returns 0!
                {
                        slotid = valueint ;
                }
                else
                {
                        parsingOk = false ;
                }

                if (parsingOk)
                {
                        int  port_ipd ;
                        if ( PortMapping::getLTPortId(port_ipd, slotid) ==  PortMapping::E_SUCCESS)
                        {


                                dbg_printf("getltportid returns fine with port id =  %d %s",port_ipd, LINEFEED) ;

                        }
                        else
                        {
                                dbg_printf("getltportid returns failure!%s", LINEFEED) ;
                        }

                }
                else
                {
                        dbg_printf("Parsing failed for getltportid!%s", LINEFEED) ;
                }
        }
        else    if (strcmp(cmdStr_i, "getfpp") == 0)
        {
                int ipdPortId = 0 ;
                unsigned long valueint = 0 ;
                int fpp = 0 ;
                bool parsingOk = true ;
                int * idx = idxPtr_i ;

                if (parsingOk == getVal(str_i, idx, &valueint)) // if dbg_scanhex is ok, it returns 0!
                {
                        ipdPortId = valueint ;
                }
                else
                {
                        parsingOk = false ;
                }

                if (parsingOk)
                {

                        if ( PortMapping::getFacePlateNumberForRemoteLt(ipdPortId, fpp) ==  PortMapping::E_SUCCESS)
                        {


                                dbg_printf("getFacePlateNumberForRemoteLt returns fine with face plate number  =  %d %s",fpp, LINEFEED) ;

                        }
                        else
                        {
                                dbg_printf("getFacePlateNumberForRemoteLt returns failure!%s", LINEFEED) ;
                        }

                }
                else
                {
                        dbg_printf("Parsing failed for getfpp!%s", LINEFEED) ;
                }
        }
        if (strcmp(cmdStr_i, "getportid4downlink") == 0)
        {
                int ipdPortId = 0 ;
                unsigned long valueint = 0 ;
                unsigned int slotid = 0 ;
                int fpp = 0 ;
                InfraMgntNt::PortMapping::IpdPortCliType porttype =  (InfraMgntNt::PortMapping::IpdPortCliType)0;
                bool parsingOk = true ;
                int * idx = idxPtr_i ;

                if (parsingOk != !dbg_scanhex(str_i, idx, &slotid)) // if dbg_scanhex is ok, it returns 0!
                {
                        parsingOk = false ;
                }

                if (parsingOk)
                {
                        if (parsingOk == getVal(str_i, idx, &valueint)) // if dbg_scanhex is ok, it returns 0!
                        {
                                fpp = valueint;
                        }
                        else
                        {
                                parsingOk = false ;
                        }
                }
                if (parsingOk)
                {
                        if (parsingOk == getVal(str_i, idx, &valueint)) // if dbg_scanhex is ok, it returns 0!
                        {
                                porttype = (InfraMgntNt::PortMapping::IpdPortCliType)valueint;
                        }
                        else
                        {
                                parsingOk = false ;
                        }
                }
                if (parsingOk)
                {
                        if ( PortMapping::getPortIdForFacePlateNumber(ipdPortId, slotid, fpp, porttype) == PortMapping::E_SUCCESS)
                        {


                                dbg_printf("getPortIdForFacePlateNumber returns fine with port number =  %d %s",ipdPortId, LINEFEED) ;

                        }
                        else
                        {
                                dbg_printf("getPortIdForFacePlateNumber returns failure!%s", LINEFEED) ;
                        }

                }
                else
                {
                        dbg_printf("Parsing failed for getfpp!%s", LINEFEED) ;
                }
        }

        else if (strcmp(cmdStr_i, "dump_portMapping") == 0)
        {
                unsigned int phyPortSlot=0;
                unsigned int facePlateNum=0;
                unsigned int phyItfType=0;
                Eqpt::LogSlotId logSlotId_l=0;
                unsigned int logPortType=0;
                unsigned int portNum;
                unsigned int retValue = PortMapping::E_FAILURE;
                dbg_printf(" LogPortNbr LogPortType PhyPortSl PhyPortType PhyPortNbr LSMSlot \n\r");
                dbg_printf("=================================================================\n\r");
                for(portNum=1;portNum<=MAX_IPDPORTS;portNum++)
                {
                        retValue = PortMapping::getPortMapping(portNum, &phyPortSlot, &facePlateNum, &phyItfType, &logSlotId_l,&logPortType);
                        if(retValue == PortMapping::E_SUCCESS)
                        {
                                dbg_printf("%6d %10d  \t0x%x %10d %10d  \t0x%x\r\n", portNum, logPortType, phyPortSlot, phyItfType, facePlateNum, logSlotId_l);
                        }
                }
        }
        return 1;
}
