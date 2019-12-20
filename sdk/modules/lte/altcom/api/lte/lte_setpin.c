/****************************************************************************
 * modules/lte/altcom/api/lte/lte_setpin.c
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <string.h>
#include <stdint.h>
#include <errno.h>

#include "lte/lte_api.h"
#include "buffpoolwrapper.h"
#include "apiutil.h"
#include "apicmd_setpinlock.h"
#include "apicmd_setpincode.h"
#include "evthdlbs.h"
#include "apicmdhdlrbs.h"
#include "altcom_callbacks.h"
#include "altcombs.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PIN_LOCK_REQ_DATA_LEN \
  (sizeof(struct apicmd_cmddat_setpinlock_s))
#define PIN_LOCK_RES_DATA_LEN \
  (sizeof(struct apicmd_cmddat_setpinlockres_s))
#define PIN_CODE_REQ_DATA_LEN \
  (sizeof(struct apicmd_cmddat_setpincode_s))
#define PIN_CODE_RES_DATA_LEN \
  (sizeof(struct apicmd_cmddat_setpincoderes_s))

#define SETPIN_TARGETPIN_MIN LTE_TARGET_PIN
#define SETPIN_TARGETPIN_MAX LTE_TARGET_PIN2

#define SETPIN_MIN_PIN_LEN (4)
#define SETPIN_MAX_PIN_LEN ((APICMD_SETPINLOCK_PINCODE_LEN) - 1)

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: setpin_lock_status_chg_cb
 *
 * Description:
 *   Notification status change in processing set PIN lock.
 *
 * Input Parameters:
 *  new_stat    Current status.
 *  old_stat    Preview status.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static int32_t setpin_lock_status_chg_cb(int32_t new_stat, int32_t old_stat)
{
  if (new_stat < ALTCOM_STATUS_POWER_ON)
    {
      DBGIF_LOG2_INFO("setpin_lock_status_chg_cb(%d -> %d)\n",
        old_stat, new_stat);
      altcomcallbacks_unreg_cb(APICMDID_SET_PIN_LOCK);

      return ALTCOM_STATUS_REG_CLR;
    }

  return ALTCOM_STATUS_REG_KEEP;
}

/****************************************************************************
 * Name: setpin_code_status_chg_cb
 *
 * Description:
 *   Notification status change in processing set PIN code.
 *
 * Input Parameters:
 *  new_stat    Current status.
 *  old_stat    Preview status.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static int32_t setpin_code_status_chg_cb(int32_t new_stat, int32_t old_stat)
{
  if (new_stat < ALTCOM_STATUS_POWER_ON)
    {
      DBGIF_LOG2_INFO("setpin_code_status_chg_cb(%d -> %d)\n",
        old_stat, new_stat);
      altcomcallbacks_unreg_cb(APICMDID_SET_PIN_CODE);

      return ALTCOM_STATUS_REG_CLR;
    }

  return ALTCOM_STATUS_REG_KEEP;
}

/****************************************************************************
 * Name: setpin_lock_job
 *
 * Description:
 *   This function is an API callback for set PIN lock.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void setpin_lock_job(FAR void *arg)
{
  int32_t                                  ret                = -1;
  set_pinenable_cb_t                       pinenable_callback = NULL;
  FAR struct apicmd_cmddat_setpinlockres_s *resdat            = NULL;

  resdat = (FAR struct apicmd_cmddat_setpinlockres_s *)arg;
  if (LTE_RESULT_OK > resdat->result ||
      LTE_RESULT_ERROR < resdat->result)
    {
      DBGIF_ASSERT(NULL, "Invalid response.\n");
    }

  ret = altcomcallbacks_get_unreg_cb(APICMDID_SET_PIN_LOCK,
                                     (void **)&pinenable_callback);

  if (0 == ret && pinenable_callback)
    {
      pinenable_callback(resdat->result, resdat->attemptsleft);
    }
  else
    {
      DBGIF_LOG_ERROR("Unexpected!! callback is NULL.\n");
    }

  altcom_free_cmd((FAR uint8_t *)arg);

  /* Unregistration status change callback. */

  altcomstatus_unreg_statchgcb(setpin_lock_status_chg_cb);
}

/****************************************************************************
 * Name: setpin_code_job
 *
 * Description:
 *   This function is an API callback for set PIN code.
 *
 * Input Parameters:
 *  arg    Pointer to received event.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void setpin_code_job(FAR void *arg)
{
  int32_t                                  ret                = -1;
  change_pin_cb_t                          changepin_callback = NULL;
  FAR struct apicmd_cmddat_setpincoderes_s *resdat            = NULL;

  resdat = (FAR struct apicmd_cmddat_setpincoderes_s *)arg;
  if (LTE_RESULT_OK > resdat->result ||
      LTE_RESULT_ERROR < resdat->result)
    {
      DBGIF_ASSERT(NULL, "Invalid response.\n");
    }

  ret = altcomcallbacks_get_unreg_cb(APICMDID_SET_PIN_CODE,
                                     (void **)&changepin_callback);

  if (0 == ret && changepin_callback)
    {
      changepin_callback(resdat->result, resdat->attemptsleft);
    }
  else
    {
      DBGIF_LOG_ERROR("Unexpected!! callback is NULL.\n");
    }

  altcom_free_cmd((FAR uint8_t *)arg);

  /* Unregistration status change callback. */

  altcomstatus_unreg_statchgcb(setpin_code_status_chg_cb);
}

/****************************************************************************
 * Name: lte_setpinenable_impl
 *
 * Description:
 *   Set Personal Identification Number enable.
 *
 * Input Parameters:
 *   enable        "Enable" or "Disable".
 *   pincode       Current PIN code. Minimum number of digits is 4.
 *                 Maximum number of digits is 8, end with '\0'.
 *                 (i.e. Max 9 byte)
 *   attemptsleft  Number of attempts left. Set only if failed.
 *   callback      Callback function to notify that setting of PIN
 *                 enables/disables is completed.
 *                 If the callback is NULL, operates with synchronous API,
 *                 otherwise operates with asynchronous API.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

static int32_t lte_setpinenable_impl(bool enable,
                                     int8_t *pincode, uint8_t *attemptsleft,
                                     set_pinenable_cb_t callback)
{
  int32_t                                ret;
  FAR struct apicmd_cmddat_setpinlock_s *reqbuff    = NULL;
  FAR uint8_t                           *presbuff   = NULL;
  struct apicmd_cmddat_setpinlockres_s   resbuff;
  uint16_t                               resbufflen = PIN_LOCK_RES_DATA_LEN;
  uint16_t                               reslen     = 0;
  int                                    sync       = (callback == NULL);
  uint8_t                                pinlen     = 0;

  /* Check input parameter */

  if (!pincode)
    {
      DBGIF_LOG_ERROR("Input argument is NULL.\n");
      return -EINVAL;
    }

  if (!attemptsleft && !callback)
    {
      DBGIF_LOG_ERROR("Input argument is NULL.\n");
      return -EINVAL;
    }

  pinlen = strlen((FAR char *)pincode);
  if (pinlen < SETPIN_MIN_PIN_LEN || SETPIN_MAX_PIN_LEN < pinlen)
    {
      return -EINVAL;
    }

  /* Check LTE library status */

  ret = altcombs_check_poweron_status();
  if (0 > ret)
    {
      return ret;
    }

  if (sync)
    {
      presbuff = (FAR uint8_t *)&resbuff;
    }
  else
    {
      /* Setup API callback */

      ret = altcombs_setup_apicallback(APICMDID_SET_PIN_LOCK, callback,
                                       setpin_lock_status_chg_cb);
      if (0 > ret)
        {
          return ret;
        }
    }

  /* Allocate API command buffer to send */

  reqbuff = (FAR struct apicmd_cmddat_setpinlock_s *)
              apicmdgw_cmd_allocbuff(APICMDID_SET_PIN_LOCK,
                                     PIN_LOCK_REQ_DATA_LEN);
  if (!reqbuff)
    {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Get PIN input parameters */

  reqbuff->mode = enable;
  strncpy((FAR char *)reqbuff->pincode,
          (FAR char *)pincode, sizeof(reqbuff->pincode));

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)reqbuff, presbuff,
                      resbufflen, &reslen, SYS_TIMEO_FEVR);
  altcom_free_cmd((FAR uint8_t *)reqbuff);

  if (0 > ret)
    {
      goto errout;
    }

  ret = 0;

  if (sync)
    {
      ret = (LTE_RESULT_OK == resbuff.result) ? 0 : -EPROTO;
      if (ret != 0)
        {
          *attemptsleft = resbuff.attemptsleft;
        }
    }

  return ret;

errout:
  if (!sync)
    {
      altcombs_teardown_apicallback(APICMDID_SET_PIN_LOCK,
                                    setpin_lock_status_chg_cb);
    }
  return ret;
}

/****************************************************************************
 * Name: lte_changepin_impl
 *
 * Description:
 *   Change Personal Identification Number.
 *
 * Input Parameters:
 *   target_pin   Target of change PIN.
 *   pincode      Current PIN code. Minimum number of digits is 4.
 *                Maximum number of digits is 8, end with '\0'.
 *                (i.e. Max 9 byte)
 *   new_pincode  New PIN code. Minimum number of digits is 4.
 *                Maximum number of digits is 8, end with '\0'.
 *                (i.e. Max 9 byte)
 *   attemptsleft Number of attempts left. Set only if failed.
 *   callback     Callback function to notify that
 *                change of PIN is completed.
 *                If the callback is NULL, operates with synchronous API,
 *                otherwise operates with asynchronous API.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

static int32_t lte_changepin_impl(int8_t target_pin, int8_t *pincode,
                                  int8_t *new_pincode, uint8_t *attemptsleft,
                                  change_pin_cb_t callback)
{
  int32_t                                ret;
  FAR struct apicmd_cmddat_setpincode_s *reqbuff    = NULL;
  FAR uint8_t                           *presbuff   = NULL;
  struct apicmd_cmddat_setpincoderes_s   resbuff;
  uint16_t                               resbufflen = PIN_CODE_RES_DATA_LEN;
  uint16_t                               reslen     = 0;
  int                                    sync       = (callback == NULL);
  uint8_t                                pinlen     = 0;

  /* Return error if argument is NULL */

  if (!pincode || !new_pincode)
    {
      DBGIF_LOG_ERROR("Input argument is NULL.\n");
      return -EINVAL;
    }
  else if (!callback && !attemptsleft)
    {
      DBGIF_LOG_ERROR("Input argument is NULL.\n");
      return -EINVAL;
    }

  if (SETPIN_TARGETPIN_MIN > target_pin || SETPIN_TARGETPIN_MAX < target_pin)
    {
      DBGIF_LOG1_ERROR("Unsupport change type. type:%d\n", target_pin);
      return -EINVAL;
    }

  pinlen = strlen((FAR char *)pincode);
  if (pinlen < SETPIN_MIN_PIN_LEN || SETPIN_MAX_PIN_LEN < pinlen)
    {
      return -EINVAL;
    }

  pinlen = strlen((FAR char *)new_pincode);
  if (pinlen < SETPIN_MIN_PIN_LEN || SETPIN_MAX_PIN_LEN < pinlen)
    {
      return -EINVAL;
    }

  /* Check LTE library status */

  ret = altcombs_check_poweron_status();
  if (0 > ret)
    {
      return ret;
    }

  if (sync)
    {
      presbuff = (FAR uint8_t *)&resbuff;
    }
  else
    {
      /* Setup API callback */

      ret = altcombs_setup_apicallback(APICMDID_SET_PIN_CODE, callback,
                                       setpin_code_status_chg_cb);
      if (0 > ret)
        {
          return ret;
        }
    }

  /* Allocate API command buffer to send */

  reqbuff = (FAR struct apicmd_cmddat_setpincode_s *)
              apicmdgw_cmd_allocbuff(APICMDID_SET_PIN_CODE,
                                     PIN_CODE_REQ_DATA_LEN);
  if (!reqbuff)
    {
      DBGIF_LOG_ERROR("Failed to allocate command buffer.\n");
      ret = -ENOMEM;
      goto errout;
    }

  if (LTE_TARGET_PIN == target_pin)
    {
      reqbuff->chgtype = APICMD_SETPINCODE_CHGTYPE_PIN;
    }
  else
    {
      reqbuff->chgtype = APICMD_SETPINCODE_CHGTYPE_PIN2;
    }

  strncpy((FAR char *)reqbuff->pincode,
          (FAR char *)pincode, sizeof(reqbuff->pincode));
  strncpy((FAR char *)reqbuff->newpincode,
          (FAR char *)new_pincode, sizeof(reqbuff->newpincode));

  /* Send API command to modem */

  ret = apicmdgw_send((FAR uint8_t *)reqbuff, presbuff,
                      resbufflen, &reslen, SYS_TIMEO_FEVR);
  altcom_free_cmd((FAR uint8_t *)reqbuff);

  if (0 > ret)
    {
      goto errout;
    }

  ret = 0;

  if (sync)
    {
      ret = (LTE_RESULT_OK == resbuff.result) ? 0 : -EPROTO;
      if (ret != 0)
        {
          *attemptsleft = resbuff.attemptsleft;
        }
    }

  return ret;

errout:
  if (!sync)
    {
      altcombs_teardown_apicallback(APICMDID_SET_PIN_CODE,
                                    setpin_code_status_chg_cb);
    }
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_set_pinenable_sync
 *
 * Description:
 *   Set Personal Identification Number enable.
 *
 * Input Parameters:
 *   enable        "Enable" or "Disable".
 *   pincode       Current PIN code. Minimum number of digits is 4.
 *                 Maximum number of digits is 8, end with '\0'.
 *                 (i.e. Max 9 byte)
 *   attemptsleft  Number of attempts left. Set only if failed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_set_pinenable_sync(bool enable, int8_t *pincode,
                               uint8_t *attemptsleft)
{
  return lte_setpinenable_impl(enable, pincode, attemptsleft, NULL);
}

/****************************************************************************
 * Name: lte_set_pinenable
 *
 * Description:
 *   Set Personal Identification Number enable.
 *
 * Input Parameters:
 *   enable        "Enable" or "Disable".
 *   pincode       Current PIN code. Minimum number of digits is 4.
 *                 Maximum number of digits is 8, end with '\0'.
 *                 (i.e. Max 9 byte)
 *   callback      Callback function to notify that setting of PIN
 *                 enables/disables is completed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_set_pinenable(bool enable, int8_t *pincode,
                          set_pinenable_cb_t callback)
{
  return lte_setpinenable_impl(enable, pincode, NULL, callback);
}

/****************************************************************************
 * Name: lte_change_pin_sync
 *
 * Description:
 *   Change Personal Identification Number.
 *
 * Input Parameters:
 *   target_pin   Target of change PIN.
 *   pincode      Current PIN code. Minimum number of digits is 4.
 *                Maximum number of digits is 8, end with '\0'.
 *                (i.e. Max 9 byte)
 *   new_pincode  New PIN code. Minimum number of digits is 4.
 *                Maximum number of digits is 8, end with '\0'.
 *                (i.e. Max 9 byte)
 *   attemptsleft Number of attempts left. Set only if failed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_change_pin_sync(int8_t target_pin, int8_t *pincode,
                            int8_t *new_pincode, uint8_t *attemptsleft)
{
  return lte_changepin_impl(target_pin, pincode, new_pincode, attemptsleft,
                            NULL);
}

/****************************************************************************
 * Name: lte_change_pin
 *
 * Description:
 *   Change Personal Identification Number.
 *
 * Input Parameters:
 *   target_pin   Target of change PIN.
 *   pincode      Current PIN code. Minimum number of digits is 4.
 *                Maximum number of digits is 8, end with '\0'.
 *                (i.e. Max 9 byte)
 *   new_pincode  New PIN code. Minimum number of digits is 4.
 *                Maximum number of digits is 8, end with '\0'.
 *                (i.e. Max 9 byte)
 *   callback     Callback function to notify that
 *                change of PIN is completed.
 *
 * Returned Value:
 *   On success, 0 is returned.
 *   On failure, negative value is returned according to <errno.h>.
 *
 ****************************************************************************/

int32_t lte_change_pin(int8_t target_pin, int8_t *pincode,
                       int8_t *new_pincode, change_pin_cb_t callback)
{
  return lte_changepin_impl(target_pin, pincode, new_pincode, NULL, callback);
}

/****************************************************************************
 * Name: apicmdhdlr_setpin
 *
 * Description:
 *   This function is an API command handler for set PIN set result.
 *
 * Input Parameters:
 *  evt    Pointer to received event.
 *  evlen  Length of received event.
 *
 * Returned Value:
 *   If the API command ID matches APICMDID_SET_PIN_LOCK_RES or
 *   APICMDID_SET_PIN_CODE_RES, EVTHDLRC_STARTHANDLE is returned.
 *   Otherwise it returns EVTHDLRC_UNSUPPORTEDEVENT. If an internal error is
 *   detected, EVTHDLRC_INTERNALERROR is returned.
 *
 ****************************************************************************/

enum evthdlrc_e apicmdhdlr_setpin(FAR uint8_t *evt, uint32_t evlen)
{
  enum evthdlrc_e ret;

  ret = apicmdhdlrbs_do_runjob(
    evt, APICMDID_CONVERT_RES(APICMDID_SET_PIN_LOCK), setpin_lock_job);
  if (EVTHDLRC_UNSUPPORTEDEVENT == ret)
    {
      ret = apicmdhdlrbs_do_runjob(
        evt, APICMDID_CONVERT_RES(APICMDID_SET_PIN_CODE), setpin_code_job);
    }

  return ret;
}
