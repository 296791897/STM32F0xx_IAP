
/******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2016 STMicroelectronics</center></h2>
  *
  * Licensed under ST MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/myliberty
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/

/*
 *      PROJECT:   ST25R391x firmware
 *      $Revision: $
 *      LANGUAGE:  ISO C99
 */
 
/*! \file rfal_dpo.c
 *
 *  \author Martin Zechleitner
 *
 *  \brief Functions to manage and set dynamic power settings.
 *  
 */

/*
 ******************************************************************************
 * INCLUDES
 ******************************************************************************
 */
#include "rfal_dpoTbl.h"
#include "rfal_dpo.h"
#include "platform.h"
#include "rfal_rf.h"
#include "rfal_chip.h"
#include "utils.h"


/*
 ******************************************************************************
 * ENABLE SWITCH
 ******************************************************************************
 */

#ifndef RFAL_FEATURE_DPO
    #error " RFAL: Module configuration missing. Please enable/disable Dynamic Power module by setting: RFAL_FEATURE_DPO "
#endif

#if RFAL_FEATURE_DPO

rfalDpoMeasureFunc  measureCallback = NULL; 
    
/*
 ******************************************************************************
 * LOCAL DATA TYPES
 ******************************************************************************
 */

static bool     gRfalDpoIsEnabled = false;  // by default DPO is disabled
static uint8_t* gRfalCurrentDpo;
static uint8_t  gRfalDpoTableEntries;
static uint8_t  gRfalDpo[RFAL_DPO_TABLE_SIZE_MAX];
static uint8_t  gRfalTableEntry;

/*
 ******************************************************************************
 * GLOBAL FUNCTIONS
 ******************************************************************************
 */
void rfalDpoInitialize( void )
{
    /* Use the default Dynamic Power values */
    gRfalCurrentDpo = (uint8_t*) rfalDpoDefaultSettings;
    gRfalDpoTableEntries = (sizeof(rfalDpoDefaultSettings) / RFAL_DPO_TABLE_PAPAMETER);
    
    ST_MEMCPY( gRfalDpo, gRfalCurrentDpo, sizeof(rfalDpoDefaultSettings) );
    
    /* by default use amplitude measurement */
    measureCallback = rfalChipMeasureAmplitude;
    
    gRfalTableEntry = 0;
}

void rfalDpoSetMeasureCallback( rfalDpoMeasureFunc pMeasureFunc )
{
  measureCallback = pMeasureFunc;
}

/*******************************************************************************/
ReturnCode rfalDpoTableWrite( rfalDpoEntry* powerTbl, uint8_t powerTblEntries )
{
    uint8_t entry = 0;
    
    // check if the table size parameter is too big
    if( (powerTblEntries * RFAL_DPO_TABLE_PAPAMETER) > RFAL_DPO_TABLE_SIZE_MAX)
    {
        return ERR_NOMEM;
    }
    
    // check if the first increase entry is 0xFF
    if( (powerTblEntries == 0) || (powerTbl == NULL) )
    {
        return ERR_PARAM;
    }
                
    // check if the entries of the dynamic power table are valid
    for (entry = 0; entry < powerTblEntries; entry++)
    {
        if(powerTbl[entry].inc < powerTbl[entry].dec)
        {
            return ERR_PARAM;
        }
    }
    
    // copy the data set 
    ST_MEMCPY( gRfalDpo, powerTbl, (powerTblEntries * RFAL_DPO_TABLE_PAPAMETER) );
    gRfalCurrentDpo = gRfalDpo;
    gRfalDpoTableEntries = powerTblEntries;
    
    if(gRfalTableEntry > powerTblEntries)
      gRfalTableEntry = (powerTblEntries - 1); // is always greater then zero, otherwise we already returned ERR_PARAM
    
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode rfalDpoTableRead( rfalDpoEntry* tblBuf, uint8_t tblBufEntries, uint8_t* tableEntries )
{
    /* wrong request */
    if( (tblBuf == NULL) || (tblBufEntries < gRfalDpoTableEntries) || (tableEntries == NULL) )
    {
        return ERR_PARAM;
    }
        
    /* Copy the whole Table to the given buffer */
    ST_MEMCPY( tblBuf, gRfalCurrentDpo, (tblBufEntries * RFAL_DPO_TABLE_PAPAMETER) );
    *tableEntries = gRfalDpoTableEntries;
    
    return ERR_NONE;
}

/*******************************************************************************/
ReturnCode rfalDpoAdjust(void)
{
    uint8_t refValue = 0;
    rfalDpoEntry* dpoTable = (rfalDpoEntry*) gRfalCurrentDpo;    
    
    /* Check if the Power Adjustment is disabled */    
    if( (gRfalCurrentDpo == NULL) || (!gRfalDpoIsEnabled) )
        return ERR_PARAM;
        
    /* Check if the callback to the measurement methode is proper set */
    if(measureCallback == NULL)
      return ERR_PARAM;
      
    /* Measure reference value*/
    if(ERR_NONE != measureCallback(&refValue))
        return ERR_PARAM;

    
    /* increase the output power */
    if( refValue >= dpoTable[gRfalTableEntry].inc )
    {
        
        /* the top of the table represents the highest amplitude value*/
        if( gRfalTableEntry == 0 )
        {
            /* check if the maximum driver value has been reached */
            return ERR_NONE;
        }
        /* go up in the table to decrease the driver resistance */
        gRfalTableEntry--;
    }
    else
    {
        /* decrease the output power */
        if(refValue <= dpoTable[gRfalTableEntry].dec)
        {
            /* The bottom is the highest possible value */
            if( (gRfalTableEntry + 1) >= gRfalDpoTableEntries)
            {
                /* check if the minimum driver value has been reached */
                return ERR_NONE;
            }
            /* go down in the table to increase the driver resistance */
            gRfalTableEntry++;
        }
        else
        {
            // do not write the driver again with the same value            
        }
    }
    
    /* get the new value for RFO resistance form the table and apply the new RFO resistance setting */ 
    rfalChipSetModulatedRFO( dpoTable[gRfalTableEntry].rfoRes );
    return ERR_NONE;
}

/*******************************************************************************/
rfalDpoEntry* rfalDpoGetCurrentTableEntry(void)
{
    rfalDpoEntry* dpoTable = (rfalDpoEntry*) gRfalCurrentDpo; 
    return &dpoTable[gRfalTableEntry];
}

/*******************************************************************************/
void rfalDpoSetEnabled( bool enable )
{
    gRfalDpoIsEnabled = enable;
}


/*******************************************************************************/
bool rfalDpoIsEnabled(void)
{
    return gRfalDpoIsEnabled;
}

#endif /* RFAL_FEATURE_DPO */
