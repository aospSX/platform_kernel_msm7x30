/*
 * Copyright (c) 2005-2010 Brocade Communications Systems, Inc.
 * All rights reserved
 * www.brocade.com
 *
 * Linux driver for Brocade Fibre Channel Host Bus Adapter.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License (GPL) Version 2 as
 * published by the Free Software Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/**
 *  rport.c Remote port implementation.
 */

#include "bfa_fcs.h"
#include "bfa_fcbuild.h"
#include "bfad_drv.h"

BFA_TRC_FILE(FCS, RPORT);

static u32
bfa_fcs_rport_del_timeout = BFA_FCS_RPORT_DEF_DEL_TIMEOUT * 1000;
	 /* In millisecs */
/*
 * forward declarations
 */
static struct bfa_fcs_rport_s *bfa_fcs_rport_alloc(
		struct bfa_fcs_lport_s *port, wwn_t pwwn, u32 rpid);
static void	bfa_fcs_rport_free(struct bfa_fcs_rport_s *rport);
static void	bfa_fcs_rport_hal_online(struct bfa_fcs_rport_s *rport);
static void	bfa_fcs_rport_online_action(struct bfa_fcs_rport_s *rport);
static void	bfa_fcs_rport_offline_action(struct bfa_fcs_rport_s *rport);
static void	bfa_fcs_rport_update(struct bfa_fcs_rport_s *rport,
					struct fc_logi_s *plogi);
static void	bfa_fcs_rport_timeout(void *arg);
static void	bfa_fcs_rport_send_plogi(void *rport_cbarg,
					 struct bfa_fcxp_s *fcxp_alloced);
static void	bfa_fcs_rport_send_plogiacc(void *rport_cbarg,
					struct bfa_fcxp_s *fcxp_alloced);
static void	bfa_fcs_rport_plogi_response(void *fcsarg,
				struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs);
static void	bfa_fcs_rport_send_adisc(void *rport_cbarg,
					 struct bfa_fcxp_s *fcxp_alloced);
static void	bfa_fcs_rport_adisc_response(void *fcsarg,
				struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs);
static void	bfa_fcs_rport_send_nsdisc(void *rport_cbarg,
					 struct bfa_fcxp_s *fcxp_alloced);
static void	bfa_fcs_rport_gidpn_response(void *fcsarg,
				struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs);
static void	bfa_fcs_rport_gpnid_response(void *fcsarg,
				struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs);
static void	bfa_fcs_rport_send_logo(void *rport_cbarg,
					struct bfa_fcxp_s *fcxp_alloced);
static void	bfa_fcs_rport_send_logo_acc(void *rport_cbarg);
static void	bfa_fcs_rport_process_prli(struct bfa_fcs_rport_s *rport,
					struct fchs_s *rx_fchs, u16 len);
static void	bfa_fcs_rport_send_ls_rjt(struct bfa_fcs_rport_s *rport,
				struct fchs_s *rx_fchs, u8 reason_code,
					  u8 reason_code_expl);
static void	bfa_fcs_rport_process_adisc(struct bfa_fcs_rport_s *rport,
				struct fchs_s *rx_fchs, u16 len);
static void bfa_fcs_rport_send_prlo_acc(struct bfa_fcs_rport_s *rport);
/**
 *  fcs_rport_sm FCS rport state machine events
 */

enum rport_event {
	RPSM_EVENT_PLOGI_SEND	= 1,	/*  new rport; start with PLOGI */
	RPSM_EVENT_PLOGI_RCVD	= 2,	/*  Inbound PLOGI from remote port */
	RPSM_EVENT_PLOGI_COMP	= 3,	/*  PLOGI completed to rport	*/
	RPSM_EVENT_LOGO_RCVD	= 4,	/*  LOGO from remote device	*/
	RPSM_EVENT_LOGO_IMP	= 5,	/*  implicit logo for SLER	*/
	RPSM_EVENT_FCXP_SENT	= 6,	/*  Frame from has been sent	*/
	RPSM_EVENT_DELETE	= 7,	/*  RPORT delete request	*/
	RPSM_EVENT_SCN		= 8,	/*  state change notification	*/
	RPSM_EVENT_ACCEPTED	= 9,	/*  Good response from remote device */
	RPSM_EVENT_FAILED	= 10,	/*  Request to rport failed.	*/
	RPSM_EVENT_TIMEOUT	= 11,	/*  Rport SM timeout event	*/
	RPSM_EVENT_HCB_ONLINE  = 12,	/*  BFA rport online callback	*/
	RPSM_EVENT_HCB_OFFLINE = 13,	/*  BFA rport offline callback	*/
	RPSM_EVENT_FC4_OFFLINE = 14,	/*  FC-4 offline complete	*/
	RPSM_EVENT_ADDRESS_CHANGE = 15,	/*  Rport's PID has changed	*/
	RPSM_EVENT_ADDRESS_DISC = 16,	/*  Need to Discover rport's PID */
	RPSM_EVENT_PRLO_RCVD   = 17,	/*  PRLO from remote device	*/
	RPSM_EVENT_PLOGI_RETRY = 18,	/*  Retry PLOGI continously */
};

static void	bfa_fcs_rport_sm_uninit(struct bfa_fcs_rport_s *rport,
					enum rport_event event);
static void	bfa_fcs_rport_sm_plogi_sending(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_plogiacc_sending(struct bfa_fcs_rport_s *rport,
						  enum rport_event event);
static void	bfa_fcs_rport_sm_plogi_retry(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_plogi(struct bfa_fcs_rport_s *rport,
					enum rport_event event);
static void	bfa_fcs_rport_sm_hal_online(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_online(struct bfa_fcs_rport_s *rport,
					enum rport_event event);
static void	bfa_fcs_rport_sm_nsquery_sending(struct bfa_fcs_rport_s *rport,
						 enum rport_event event);
static void	bfa_fcs_rport_sm_nsquery(struct bfa_fcs_rport_s *rport,
					 enum rport_event event);
static void	bfa_fcs_rport_sm_adisc_sending(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_adisc(struct bfa_fcs_rport_s *rport,
					enum rport_event event);
static void	bfa_fcs_rport_sm_fc4_logorcv(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_fc4_logosend(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_fc4_offline(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_hcb_offline(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_hcb_logorcv(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_hcb_logosend(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_logo_sending(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_offline(struct bfa_fcs_rport_s *rport,
					 enum rport_event event);
static void	bfa_fcs_rport_sm_nsdisc_sending(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_nsdisc_retry(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_nsdisc_sent(struct bfa_fcs_rport_s *rport,
						enum rport_event event);
static void	bfa_fcs_rport_sm_nsdisc_sent(struct bfa_fcs_rport_s *rport,
						enum rport_event event);

static struct bfa_sm_table_s rport_sm_table[] = {
	{BFA_SM(bfa_fcs_rport_sm_uninit), BFA_RPORT_UNINIT},
	{BFA_SM(bfa_fcs_rport_sm_plogi_sending), BFA_RPORT_PLOGI},
	{BFA_SM(bfa_fcs_rport_sm_plogiacc_sending), BFA_RPORT_ONLINE},
	{BFA_SM(bfa_fcs_rport_sm_plogi_retry), BFA_RPORT_PLOGI_RETRY},
	{BFA_SM(bfa_fcs_rport_sm_plogi), BFA_RPORT_PLOGI},
	{BFA_SM(bfa_fcs_rport_sm_hal_online), BFA_RPORT_ONLINE},
	{BFA_SM(bfa_fcs_rport_sm_online), BFA_RPORT_ONLINE},
	{BFA_SM(bfa_fcs_rport_sm_nsquery_sending), BFA_RPORT_NSQUERY},
	{BFA_SM(bfa_fcs_rport_sm_nsquery), BFA_RPORT_NSQUERY},
	{BFA_SM(bfa_fcs_rport_sm_adisc_sending), BFA_RPORT_ADISC},
	{BFA_SM(bfa_fcs_rport_sm_adisc), BFA_RPORT_ADISC},
	{BFA_SM(bfa_fcs_rport_sm_fc4_logorcv), BFA_RPORT_LOGORCV},
	{BFA_SM(bfa_fcs_rport_sm_fc4_logosend), BFA_RPORT_LOGO},
	{BFA_SM(bfa_fcs_rport_sm_fc4_offline), BFA_RPORT_OFFLINE},
	{BFA_SM(bfa_fcs_rport_sm_hcb_offline), BFA_RPORT_OFFLINE},
	{BFA_SM(bfa_fcs_rport_sm_hcb_logorcv), BFA_RPORT_LOGORCV},
	{BFA_SM(bfa_fcs_rport_sm_hcb_logosend), BFA_RPORT_LOGO},
	{BFA_SM(bfa_fcs_rport_sm_logo_sending), BFA_RPORT_LOGO},
	{BFA_SM(bfa_fcs_rport_sm_offline), BFA_RPORT_OFFLINE},
	{BFA_SM(bfa_fcs_rport_sm_nsdisc_sending), BFA_RPORT_NSDISC},
	{BFA_SM(bfa_fcs_rport_sm_nsdisc_retry), BFA_RPORT_NSDISC},
	{BFA_SM(bfa_fcs_rport_sm_nsdisc_sent), BFA_RPORT_NSDISC},
};

/**
 *		Beginning state.
 */
static void
bfa_fcs_rport_sm_uninit(struct bfa_fcs_rport_s *rport, enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_PLOGI_SEND:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi_sending);
		rport->plogi_retries = 0;
		bfa_fcs_rport_send_plogi(rport, NULL);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_fcs_rport_hal_online(rport);
		break;

	case RPSM_EVENT_ADDRESS_CHANGE:
	case RPSM_EVENT_ADDRESS_DISC:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;
	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		PLOGI is being sent.
 */
static void
bfa_fcs_rport_sm_plogi_sending(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FCXP_SENT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_ADDRESS_CHANGE:
	case RPSM_EVENT_SCN:
		/* query the NS */
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;


	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		PLOGI is being sent.
 */
static void
bfa_fcs_rport_sm_plogiacc_sending(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FCXP_SENT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_fcs_rport_hal_online(rport);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
	case RPSM_EVENT_SCN:
		/**
		 * Ignore, SCN is possibly online notification.
		 */
		break;

	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;

	case RPSM_EVENT_HCB_OFFLINE:
		/**
		 * Ignore BFA callback, on a PLOGI receive we call bfa offline.
		 */
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		PLOGI is sent.
 */
static void
bfa_fcs_rport_sm_plogi_retry(struct bfa_fcs_rport_s *rport,
			enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_TIMEOUT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi_sending);
		bfa_fcs_rport_send_plogi(rport, NULL);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PRLO_RCVD:
	case RPSM_EVENT_LOGO_RCVD:
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_ADDRESS_CHANGE:
	case RPSM_EVENT_SCN:
		bfa_timer_stop(&rport->timer);
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_timer_stop(&rport->timer);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_hal_online(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		PLOGI is sent.
 */
static void
bfa_fcs_rport_sm_plogi(struct bfa_fcs_rport_s *rport, enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_ACCEPTED:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		rport->plogi_retries = 0;
		bfa_fcs_rport_hal_online(rport);
		break;

	case RPSM_EVENT_LOGO_RCVD:
		bfa_fcs_rport_send_logo_acc(rport);
		/*
		 * !! fall through !!
		 */
	case RPSM_EVENT_PRLO_RCVD:
		if (rport->prlo == BFA_TRUE)
			bfa_fcs_rport_send_prlo_acc(rport);

		bfa_fcxp_discard(rport->fcxp);
		/*
		 * !! fall through !!
		 */
	case RPSM_EVENT_FAILED:
		if (rport->plogi_retries < BFA_FCS_RPORT_MAX_RETRIES) {
			rport->plogi_retries++;
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi_retry);
			bfa_timer_start(rport->fcs->bfa, &rport->timer,
					bfa_fcs_rport_timeout, rport,
					BFA_FCS_RETRY_TIMEOUT);
		} else {
			bfa_stats(rport->port, rport_del_max_plogi_retry);
			rport->pid = 0;
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
			bfa_timer_start(rport->fcs->bfa, &rport->timer,
					bfa_fcs_rport_timeout, rport,
					bfa_fcs_rport_del_timeout);
		}
		break;

	case	RPSM_EVENT_PLOGI_RETRY:
		rport->plogi_retries = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi_retry);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				(FC_RA_TOV * 1000));
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_fcxp_discard(rport->fcxp);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;

	case RPSM_EVENT_ADDRESS_CHANGE:
	case RPSM_EVENT_SCN:
		bfa_fcxp_discard(rport->fcxp);
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_hal_online(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		PLOGI is complete. Awaiting BFA rport online callback. FC-4s
 *		are offline.
 */
static void
bfa_fcs_rport_sm_hal_online(struct bfa_fcs_rport_s *rport,
			enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_HCB_ONLINE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_online);
		bfa_fcs_rport_online_action(rport);
		break;

	case RPSM_EVENT_PRLO_RCVD:
		break;

	case RPSM_EVENT_LOGO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_logorcv);
		bfa_rport_offline(rport->bfa_rport);
		break;

	case RPSM_EVENT_LOGO_IMP:
	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_offline);
		bfa_rport_offline(rport->bfa_rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_rport_offline(rport->bfa_rport);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_logosend);
		bfa_rport_offline(rport->bfa_rport);
		break;

	case RPSM_EVENT_SCN:
		/**
		 * @todo
		 * Ignore SCN - PLOGI just completed, FC-4 login should detect
		 * device failures.
		 */
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport is ONLINE. FC-4s active.
 */
static void
bfa_fcs_rport_sm_online(struct bfa_fcs_rport_s *rport, enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_SCN:
		if (bfa_fcs_fabric_is_switched(rport->port->fabric)) {
			bfa_sm_set_state(rport,
					 bfa_fcs_rport_sm_nsquery_sending);
			rport->ns_retries = 0;
			bfa_fcs_rport_send_nsdisc(rport, NULL);
		} else {
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_adisc_sending);
			bfa_fcs_rport_send_adisc(rport, NULL);
		}
		break;

	case RPSM_EVENT_PLOGI_RCVD:
	case RPSM_EVENT_LOGO_IMP:
	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logorcv);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		An SCN event is received in ONLINE state. NS query is being sent
 *		prior to ADISC authentication with rport. FC-4s are paused.
 */
static void
bfa_fcs_rport_sm_nsquery_sending(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FCXP_SENT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsquery);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_SCN:
		/**
		 * ignore SCN, wait for response to query itself
		 */
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logorcv);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
	case RPSM_EVENT_ADDRESS_CHANGE:
	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *	An SCN event is received in ONLINE state. NS query is sent to rport.
 *	FC-4s are paused.
 */
static void
bfa_fcs_rport_sm_nsquery(struct bfa_fcs_rport_s *rport, enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_ACCEPTED:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_adisc_sending);
		bfa_fcs_rport_send_adisc(rport, NULL);
		break;

	case RPSM_EVENT_FAILED:
		rport->ns_retries++;
		if (rport->ns_retries < BFA_FCS_RPORT_MAX_RETRIES) {
			bfa_sm_set_state(rport,
					 bfa_fcs_rport_sm_nsquery_sending);
			bfa_fcs_rport_send_nsdisc(rport, NULL);
		} else {
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
			bfa_fcs_rport_offline_action(rport);
		}
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_SCN:
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logorcv);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_PLOGI_COMP:
	case RPSM_EVENT_ADDRESS_CHANGE:
	case RPSM_EVENT_PLOGI_RCVD:
	case RPSM_EVENT_LOGO_IMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_offline_action(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *	An SCN event is received in ONLINE state. ADISC is being sent for
 *	authenticating with rport. FC-4s are paused.
 */
static void
bfa_fcs_rport_sm_adisc_sending(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FCXP_SENT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_adisc);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_LOGO_IMP:
	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logorcv);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_SCN:
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_offline_action(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		An SCN event is received in ONLINE state. ADISC is to rport.
 *		FC-4s are paused.
 */
static void
bfa_fcs_rport_sm_adisc(struct bfa_fcs_rport_s *rport, enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_ACCEPTED:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_online);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		/**
		 * Too complex to cleanup FC-4 & rport and then acc to PLOGI.
		 * At least go offline when a PLOGI is received.
		 */
		bfa_fcxp_discard(rport->fcxp);
		/*
		 * !!! fall through !!!
		 */

	case RPSM_EVENT_FAILED:
	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_SCN:
		/**
		 * already processing RSCN
		 */
		break;

	case RPSM_EVENT_LOGO_IMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_offline);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_offline_action(rport);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logorcv);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_offline_action(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport has sent LOGO. Awaiting FC-4 offline completion callback.
 */
static void
bfa_fcs_rport_sm_fc4_logorcv(struct bfa_fcs_rport_s *rport,
			enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FC4_OFFLINE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_logorcv);
		bfa_rport_offline(rport->bfa_rport);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
	case RPSM_EVENT_ADDRESS_CHANGE:
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		LOGO needs to be sent to rport. Awaiting FC-4 offline completion
 *		callback.
 */
static void
bfa_fcs_rport_sm_fc4_logosend(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FC4_OFFLINE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_logosend);
		bfa_rport_offline(rport->bfa_rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *	Rport is going offline. Awaiting FC-4 offline completion callback.
 */
static void
bfa_fcs_rport_sm_fc4_offline(struct bfa_fcs_rport_s *rport,
			enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FC4_OFFLINE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_offline);
		bfa_rport_offline(rport->bfa_rport);
		break;

	case RPSM_EVENT_SCN:
	case RPSM_EVENT_LOGO_IMP:
	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
	case RPSM_EVENT_ADDRESS_CHANGE:
		/**
		 * rport is already going offline.
		 * SCN - ignore and wait till transitioning to offline state
		 */
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_fc4_logosend);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport is offline. FC-4s are offline. Awaiting BFA rport offline
 *		callback.
 */
static void
bfa_fcs_rport_sm_hcb_offline(struct bfa_fcs_rport_s *rport,
				enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_HCB_OFFLINE:
	case RPSM_EVENT_ADDRESS_CHANGE:
		if (bfa_fcs_lport_is_online(rport->port)) {
			if (bfa_fcs_fabric_is_switched(rport->port->fabric)) {
				bfa_sm_set_state(rport,
					bfa_fcs_rport_sm_nsdisc_sending);
				rport->ns_retries = 0;
				bfa_fcs_rport_send_nsdisc(rport, NULL);
			} else {
				bfa_sm_set_state(rport,
					bfa_fcs_rport_sm_plogi_sending);
				rport->plogi_retries = 0;
				bfa_fcs_rport_send_plogi(rport, NULL);
			}
		} else {
			rport->pid = 0;
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
			bfa_timer_start(rport->fcs->bfa, &rport->timer,
					bfa_fcs_rport_timeout, rport,
					bfa_fcs_rport_del_timeout);
		}
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_SCN:
	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		/**
		 * Ignore, already offline.
		 */
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport is offline. FC-4s are offline. Awaiting BFA rport offline
 *		callback to send LOGO accept.
 */
static void
bfa_fcs_rport_sm_hcb_logorcv(struct bfa_fcs_rport_s *rport,
			enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_HCB_OFFLINE:
	case RPSM_EVENT_ADDRESS_CHANGE:
		if (rport->pid && (rport->prlo == BFA_TRUE))
			bfa_fcs_rport_send_prlo_acc(rport);
		if (rport->pid && (rport->prlo == BFA_FALSE))
			bfa_fcs_rport_send_logo_acc(rport);
		/*
		 * If the lport is online and if the rport is not a well
		 * known address port,
		 * we try to re-discover the r-port.
		 */
		if (bfa_fcs_lport_is_online(rport->port) &&
			(!BFA_FCS_PID_IS_WKA(rport->pid))) {
			bfa_sm_set_state(rport,
				bfa_fcs_rport_sm_nsdisc_sending);
			rport->ns_retries = 0;
			bfa_fcs_rport_send_nsdisc(rport, NULL);
		} else {
			/*
			 * if it is not a well known address, reset the
			 * pid to 0.
			 */
			if (!BFA_FCS_PID_IS_WKA(rport->pid))
				rport->pid = 0;
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
			bfa_timer_start(rport->fcs->bfa, &rport->timer,
					bfa_fcs_rport_timeout, rport,
					bfa_fcs_rport_del_timeout);
		}
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_logosend);
		break;

	case RPSM_EVENT_LOGO_IMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hcb_offline);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		/**
		 * Ignore - already processing a LOGO.
		 */
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport is being deleted. FC-4s are offline.
 *  Awaiting BFA rport offline
 *		callback to send LOGO.
 */
static void
bfa_fcs_rport_sm_hcb_logosend(struct bfa_fcs_rport_s *rport,
		 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_HCB_OFFLINE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_logo_sending);
		bfa_fcs_rport_send_logo(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
	case RPSM_EVENT_ADDRESS_CHANGE:
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport is being deleted. FC-4s are offline. LOGO is being sent.
 */
static void
bfa_fcs_rport_sm_logo_sending(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FCXP_SENT:
		/* Once LOGO is sent, we donot wait for the response */
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_SCN:
	case RPSM_EVENT_ADDRESS_CHANGE:
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_free(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport is offline. FC-4s are offline. BFA rport is offline.
 *		Timer active to delete stale rport.
 */
static void
bfa_fcs_rport_sm_offline(struct bfa_fcs_rport_s *rport, enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_TIMEOUT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_SCN:
	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		bfa_timer_stop(&rport->timer);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
	case RPSM_EVENT_LOGO_IMP:
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_hal_online(rport);
		break;

	case RPSM_EVENT_PLOGI_SEND:
		bfa_timer_stop(&rport->timer);
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi_sending);
		rport->plogi_retries = 0;
		bfa_fcs_rport_send_plogi(rport, NULL);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *	Rport address has changed. Nameserver discovery request is being sent.
 */
static void
bfa_fcs_rport_sm_nsdisc_sending(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_FCXP_SENT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sent);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_SCN:
	case RPSM_EVENT_LOGO_RCVD:
	case RPSM_EVENT_PRLO_RCVD:
	case RPSM_EVENT_PLOGI_SEND:
		break;

	case RPSM_EVENT_ADDRESS_CHANGE:
		rport->ns_retries = 0; /* reset the retry count */
		break;

	case RPSM_EVENT_LOGO_IMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rport->fcxp_wqe);
		bfa_fcs_rport_hal_online(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Nameserver discovery failed. Waiting for timeout to retry.
 */
static void
bfa_fcs_rport_sm_nsdisc_retry(struct bfa_fcs_rport_s *rport,
	 enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_TIMEOUT:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_SCN:
	case RPSM_EVENT_ADDRESS_CHANGE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_nsdisc_sending);
		bfa_timer_stop(&rport->timer);
		rport->ns_retries = 0;
		bfa_fcs_rport_send_nsdisc(rport, NULL);
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_timer_stop(&rport->timer);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;

	case RPSM_EVENT_LOGO_RCVD:
		bfa_fcs_rport_send_logo_acc(rport);
		break;
	case RPSM_EVENT_PRLO_RCVD:
		bfa_fcs_rport_send_prlo_acc(rport);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_timer_stop(&rport->timer);
		bfa_fcs_rport_hal_online(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

/**
 *		Rport address has changed. Nameserver discovery request is sent.
 */
static void
bfa_fcs_rport_sm_nsdisc_sent(struct bfa_fcs_rport_s *rport,
			enum rport_event event)
{
	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPSM_EVENT_ACCEPTED:
	case RPSM_EVENT_ADDRESS_CHANGE:
		if (rport->pid) {
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogi_sending);
			bfa_fcs_rport_send_plogi(rport, NULL);
		} else {
			bfa_sm_set_state(rport,
				 bfa_fcs_rport_sm_nsdisc_sending);
			rport->ns_retries = 0;
			bfa_fcs_rport_send_nsdisc(rport, NULL);
		}
		break;

	case RPSM_EVENT_FAILED:
		rport->ns_retries++;
		if (rport->ns_retries < BFA_FCS_RPORT_MAX_RETRIES) {
			bfa_sm_set_state(rport,
				 bfa_fcs_rport_sm_nsdisc_sending);
			bfa_fcs_rport_send_nsdisc(rport, NULL);
		} else {
			rport->pid = 0;
			bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
			bfa_timer_start(rport->fcs->bfa, &rport->timer,
					bfa_fcs_rport_timeout, rport,
					bfa_fcs_rport_del_timeout);
		};
		break;

	case RPSM_EVENT_DELETE:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_free(rport);
		break;

	case RPSM_EVENT_PLOGI_RCVD:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_plogiacc_sending);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_send_plogiacc(rport, NULL);
		break;

	case RPSM_EVENT_LOGO_IMP:
		rport->pid = 0;
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_offline);
		bfa_fcxp_discard(rport->fcxp);
		bfa_timer_start(rport->fcs->bfa, &rport->timer,
				bfa_fcs_rport_timeout, rport,
				bfa_fcs_rport_del_timeout);
		break;


	case RPSM_EVENT_PRLO_RCVD:
		bfa_fcs_rport_send_prlo_acc(rport);
		break;
	case RPSM_EVENT_SCN:
		/**
		 * ignore, wait for NS query response
		 */
		break;

	case RPSM_EVENT_LOGO_RCVD:
		/**
		 * Not logged-in yet. Accept LOGO.
		 */
		bfa_fcs_rport_send_logo_acc(rport);
		break;

	case RPSM_EVENT_PLOGI_COMP:
		bfa_sm_set_state(rport, bfa_fcs_rport_sm_hal_online);
		bfa_fcxp_discard(rport->fcxp);
		bfa_fcs_rport_hal_online(rport);
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}



/**
 *  fcs_rport_private FCS RPORT provate functions
 */

static void
bfa_fcs_rport_send_plogi(void *rport_cbarg, struct bfa_fcxp_s *fcxp_alloced)
{
	struct bfa_fcs_rport_s *rport = rport_cbarg;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s	fchs;
	int		len;
	struct bfa_fcxp_s *fcxp;

	bfa_trc(rport->fcs, rport->pwwn);

	fcxp = fcxp_alloced ? fcxp_alloced : bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp) {
		bfa_fcs_fcxp_alloc_wait(port->fcs->bfa, &rport->fcxp_wqe,
					bfa_fcs_rport_send_plogi, rport);
		return;
	}
	rport->fcxp = fcxp;

	len = fc_plogi_build(&fchs, bfa_fcxp_get_reqbuf(fcxp), rport->pid,
				bfa_fcs_lport_get_fcid(port), 0,
				port->port_cfg.pwwn, port->port_cfg.nwwn,
				bfa_fcport_get_maxfrsize(port->fcs->bfa));

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, bfa_fcs_rport_plogi_response,
			(void *)rport, FC_MAX_PDUSZ, FC_ELS_TOV);

	rport->stats.plogis++;
	bfa_sm_send_event(rport, RPSM_EVENT_FCXP_SENT);
}

static void
bfa_fcs_rport_plogi_response(void *fcsarg, struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;
	struct fc_logi_s	*plogi_rsp;
	struct fc_ls_rjt_s	*ls_rjt;
	struct bfa_fcs_rport_s *twin;
	struct list_head	*qe;

	bfa_trc(rport->fcs, rport->pwwn);

	/*
	 * Sanity Checks
	 */
	if (req_status != BFA_STATUS_OK) {
		bfa_trc(rport->fcs, req_status);
		rport->stats.plogi_failed++;
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		return;
	}

	plogi_rsp = (struct fc_logi_s *) BFA_FCXP_RSP_PLD(fcxp);

	/**
	 * Check for failure first.
	 */
	if (plogi_rsp->els_cmd.els_code != FC_ELS_ACC) {
		ls_rjt = (struct fc_ls_rjt_s *) BFA_FCXP_RSP_PLD(fcxp);

		bfa_trc(rport->fcs, ls_rjt->reason_code);
		bfa_trc(rport->fcs, ls_rjt->reason_code_expl);

		if ((ls_rjt->reason_code == FC_LS_RJT_RSN_UNABLE_TO_PERF_CMD) &&
		 (ls_rjt->reason_code_expl == FC_LS_RJT_EXP_INSUFF_RES)) {
			rport->stats.rjt_insuff_res++;
			bfa_sm_send_event(rport, RPSM_EVENT_PLOGI_RETRY);
			return;
		}

		rport->stats.plogi_rejects++;
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		return;
	}

	/**
	 * PLOGI is complete. Make sure this device is not one of the known
	 * device with a new FC port address.
	 */
	list_for_each(qe, &rport->port->rport_q) {
		twin = (struct bfa_fcs_rport_s *) qe;
		if (twin == rport)
			continue;
		if (!rport->pwwn && (plogi_rsp->port_name == twin->pwwn)) {
			bfa_trc(rport->fcs, twin->pid);
			bfa_trc(rport->fcs, rport->pid);

			/* Update plogi stats in twin */
			twin->stats.plogis  += rport->stats.plogis;
			twin->stats.plogi_rejects  +=
				 rport->stats.plogi_rejects;
			twin->stats.plogi_timeouts  +=
				 rport->stats.plogi_timeouts;
			twin->stats.plogi_failed +=
				 rport->stats.plogi_failed;
			twin->stats.plogi_rcvd	  += rport->stats.plogi_rcvd;
			twin->stats.plogi_accs++;

			bfa_fcs_rport_delete(rport);

			bfa_fcs_rport_update(twin, plogi_rsp);
			twin->pid = rsp_fchs->s_id;
			bfa_sm_send_event(twin, RPSM_EVENT_PLOGI_COMP);
			return;
		}
	}

	/**
	 * Normal login path -- no evil twins.
	 */
	rport->stats.plogi_accs++;
	bfa_fcs_rport_update(rport, plogi_rsp);
	bfa_sm_send_event(rport, RPSM_EVENT_ACCEPTED);
}

static void
bfa_fcs_rport_send_plogiacc(void *rport_cbarg, struct bfa_fcxp_s *fcxp_alloced)
{
	struct bfa_fcs_rport_s *rport = rport_cbarg;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s		fchs;
	int		len;
	struct bfa_fcxp_s *fcxp;

	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->reply_oxid);

	fcxp = fcxp_alloced ? fcxp_alloced : bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp) {
		bfa_fcs_fcxp_alloc_wait(port->fcs->bfa, &rport->fcxp_wqe,
					bfa_fcs_rport_send_plogiacc, rport);
		return;
	}
	rport->fcxp = fcxp;

	len = fc_plogi_acc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				 rport->pid, bfa_fcs_lport_get_fcid(port),
				 rport->reply_oxid, port->port_cfg.pwwn,
				 port->port_cfg.nwwn,
				 bfa_fcport_get_maxfrsize(port->fcs->bfa));

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, NULL, NULL, FC_MAX_PDUSZ, 0);

	bfa_sm_send_event(rport, RPSM_EVENT_FCXP_SENT);
}

static void
bfa_fcs_rport_send_adisc(void *rport_cbarg, struct bfa_fcxp_s *fcxp_alloced)
{
	struct bfa_fcs_rport_s *rport = rport_cbarg;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s		fchs;
	int		len;
	struct bfa_fcxp_s *fcxp;

	bfa_trc(rport->fcs, rport->pwwn);

	fcxp = fcxp_alloced ? fcxp_alloced : bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp) {
		bfa_fcs_fcxp_alloc_wait(port->fcs->bfa, &rport->fcxp_wqe,
					bfa_fcs_rport_send_adisc, rport);
		return;
	}
	rport->fcxp = fcxp;

	len = fc_adisc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp), rport->pid,
				bfa_fcs_lport_get_fcid(port), 0,
				port->port_cfg.pwwn, port->port_cfg.nwwn);

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, bfa_fcs_rport_adisc_response,
			rport, FC_MAX_PDUSZ, FC_ELS_TOV);

	rport->stats.adisc_sent++;
	bfa_sm_send_event(rport, RPSM_EVENT_FCXP_SENT);
}

static void
bfa_fcs_rport_adisc_response(void *fcsarg, struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;
	void		*pld = bfa_fcxp_get_rspbuf(fcxp);
	struct fc_ls_rjt_s	*ls_rjt;

	if (req_status != BFA_STATUS_OK) {
		bfa_trc(rport->fcs, req_status);
		rport->stats.adisc_failed++;
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		return;
	}

	if (fc_adisc_rsp_parse((struct fc_adisc_s *)pld, rsp_len, rport->pwwn,
				rport->nwwn)  == FC_PARSE_OK) {
		rport->stats.adisc_accs++;
		bfa_sm_send_event(rport, RPSM_EVENT_ACCEPTED);
		return;
	}

	rport->stats.adisc_rejects++;
	ls_rjt = pld;
	bfa_trc(rport->fcs, ls_rjt->els_cmd.els_code);
	bfa_trc(rport->fcs, ls_rjt->reason_code);
	bfa_trc(rport->fcs, ls_rjt->reason_code_expl);
	bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
}

static void
bfa_fcs_rport_send_nsdisc(void *rport_cbarg, struct bfa_fcxp_s *fcxp_alloced)
{
	struct bfa_fcs_rport_s *rport = rport_cbarg;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s	fchs;
	struct bfa_fcxp_s *fcxp;
	int		len;
	bfa_cb_fcxp_send_t cbfn;

	bfa_trc(rport->fcs, rport->pid);

	fcxp = fcxp_alloced ? fcxp_alloced : bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp) {
		bfa_fcs_fcxp_alloc_wait(port->fcs->bfa, &rport->fcxp_wqe,
					bfa_fcs_rport_send_nsdisc, rport);
		return;
	}
	rport->fcxp = fcxp;

	if (rport->pwwn) {
		len = fc_gidpn_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				bfa_fcs_lport_get_fcid(port), 0, rport->pwwn);
		cbfn = bfa_fcs_rport_gidpn_response;
	} else {
		len = fc_gpnid_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				bfa_fcs_lport_get_fcid(port), 0, rport->pid);
		cbfn = bfa_fcs_rport_gpnid_response;
	}

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, cbfn,
			(void *)rport, FC_MAX_PDUSZ, FC_FCCT_TOV);

	bfa_sm_send_event(rport, RPSM_EVENT_FCXP_SENT);
}

static void
bfa_fcs_rport_gidpn_response(void *fcsarg, struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;
	struct ct_hdr_s	*cthdr;
	struct fcgs_gidpn_resp_s	*gidpn_rsp;
	struct bfa_fcs_rport_s	*twin;
	struct list_head	*qe;

	bfa_trc(rport->fcs, rport->pwwn);

	cthdr = (struct ct_hdr_s *) BFA_FCXP_RSP_PLD(fcxp);
	cthdr->cmd_rsp_code = be16_to_cpu(cthdr->cmd_rsp_code);

	if (cthdr->cmd_rsp_code == CT_RSP_ACCEPT) {
		/* Check if the pid is the same as before. */
		gidpn_rsp = (struct fcgs_gidpn_resp_s *) (cthdr + 1);

		if (gidpn_rsp->dap == rport->pid) {
			/* Device is online  */
			bfa_sm_send_event(rport, RPSM_EVENT_ACCEPTED);
		} else {
			/*
			 * Device's PID has changed. We need to cleanup
			 * and re-login. If there is another device with
			 * the the newly discovered pid, send an scn notice
			 * so that its new pid can be discovered.
			 */
			list_for_each(qe, &rport->port->rport_q) {
				twin = (struct bfa_fcs_rport_s *) qe;
				if (twin == rport)
					continue;
				if (gidpn_rsp->dap == twin->pid) {
					bfa_trc(rport->fcs, twin->pid);
					bfa_trc(rport->fcs, rport->pid);

					twin->pid = 0;
					bfa_sm_send_event(twin,
					 RPSM_EVENT_ADDRESS_CHANGE);
				}
			}
			rport->pid = gidpn_rsp->dap;
			bfa_sm_send_event(rport, RPSM_EVENT_ADDRESS_CHANGE);
		}
		return;
	}

	/*
	 * Reject Response
	 */
	switch (cthdr->reason_code) {
	case CT_RSN_LOGICAL_BUSY:
		/*
		 * Need to retry
		 */
		bfa_sm_send_event(rport, RPSM_EVENT_TIMEOUT);
		break;

	case CT_RSN_UNABLE_TO_PERF:
		/*
		 * device doesn't exist : Start timer to cleanup this later.
		 */
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		break;

	default:
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		break;
	}
}

static void
bfa_fcs_rport_gpnid_response(void *fcsarg, struct bfa_fcxp_s *fcxp, void *cbarg,
				bfa_status_t req_status, u32 rsp_len,
				u32 resid_len, struct fchs_s *rsp_fchs)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;
	struct ct_hdr_s	*cthdr;

	bfa_trc(rport->fcs, rport->pwwn);

	cthdr = (struct ct_hdr_s *) BFA_FCXP_RSP_PLD(fcxp);
	cthdr->cmd_rsp_code = be16_to_cpu(cthdr->cmd_rsp_code);

	if (cthdr->cmd_rsp_code == CT_RSP_ACCEPT) {
		bfa_sm_send_event(rport, RPSM_EVENT_ACCEPTED);
		return;
	}

	/*
	 * Reject Response
	 */
	switch (cthdr->reason_code) {
	case CT_RSN_LOGICAL_BUSY:
		/*
		 * Need to retry
		 */
		bfa_sm_send_event(rport, RPSM_EVENT_TIMEOUT);
		break;

	case CT_RSN_UNABLE_TO_PERF:
		/*
		 * device doesn't exist : Start timer to cleanup this later.
		 */
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		break;

	default:
		bfa_sm_send_event(rport, RPSM_EVENT_FAILED);
		break;
	}
}

/**
 *	Called to send a logout to the rport.
 */
static void
bfa_fcs_rport_send_logo(void *rport_cbarg, struct bfa_fcxp_s *fcxp_alloced)
{
	struct bfa_fcs_rport_s *rport = rport_cbarg;
	struct bfa_fcs_lport_s *port;
	struct fchs_s	fchs;
	struct bfa_fcxp_s *fcxp;
	u16	len;

	bfa_trc(rport->fcs, rport->pid);

	port = rport->port;

	fcxp = fcxp_alloced ? fcxp_alloced : bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp) {
		bfa_fcs_fcxp_alloc_wait(port->fcs->bfa, &rport->fcxp_wqe,
					bfa_fcs_rport_send_logo, rport);
		return;
	}
	rport->fcxp = fcxp;

	len = fc_logo_build(&fchs, bfa_fcxp_get_reqbuf(fcxp), rport->pid,
				bfa_fcs_lport_get_fcid(port), 0,
				bfa_fcs_lport_get_pwwn(port));

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, NULL,
			rport, FC_MAX_PDUSZ, FC_ELS_TOV);

	rport->stats.logos++;
	bfa_fcxp_discard(rport->fcxp);
	bfa_sm_send_event(rport, RPSM_EVENT_FCXP_SENT);
}

/**
 *	Send ACC for a LOGO received.
 */
static void
bfa_fcs_rport_send_logo_acc(void *rport_cbarg)
{
	struct bfa_fcs_rport_s *rport = rport_cbarg;
	struct bfa_fcs_lport_s *port;
	struct fchs_s	fchs;
	struct bfa_fcxp_s *fcxp;
	u16	len;

	bfa_trc(rport->fcs, rport->pid);

	port = rport->port;

	fcxp = bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp)
		return;

	rport->stats.logo_rcvd++;
	len = fc_logo_acc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				rport->pid, bfa_fcs_lport_get_fcid(port),
				rport->reply_oxid);

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, NULL, NULL, FC_MAX_PDUSZ, 0);
}

/**
 *	brief
 *	This routine will be called by bfa_timer on timer timeouts.
 *
 *	param[in]	rport			- pointer to bfa_fcs_lport_ns_t.
 *	param[out]	rport_status	- pointer to return vport status in
 *
 *	return
 *		void
 *
 *	Special Considerations:
 *
 *	note
 */
static void
bfa_fcs_rport_timeout(void *arg)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) arg;

	rport->stats.plogi_timeouts++;
	bfa_stats(rport->port, rport_plogi_timeouts);
	bfa_sm_send_event(rport, RPSM_EVENT_TIMEOUT);
}

static void
bfa_fcs_rport_process_prli(struct bfa_fcs_rport_s *rport,
			struct fchs_s *rx_fchs, u16 len)
{
	struct bfa_fcxp_s *fcxp;
	struct fchs_s	fchs;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fc_prli_s	*prli;

	bfa_trc(port->fcs, rx_fchs->s_id);
	bfa_trc(port->fcs, rx_fchs->d_id);

	rport->stats.prli_rcvd++;

	/*
	 * We are in Initiator Mode
	 */
	prli = (struct fc_prli_s *) (rx_fchs + 1);

	if (prli->parampage.servparams.target) {
		/*
		 * PRLI from a target ?
		 * Send the Acc.
		 * PRLI sent by us will be used to transition the IT nexus,
		 * once the response is received from the target.
		 */
		bfa_trc(port->fcs, rx_fchs->s_id);
		rport->scsi_function = BFA_RPORT_TARGET;
	} else {
		bfa_trc(rport->fcs, prli->parampage.type);
		rport->scsi_function = BFA_RPORT_INITIATOR;
		bfa_fcs_itnim_is_initiator(rport->itnim);
	}

	fcxp = bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp)
		return;

	len = fc_prli_acc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				rx_fchs->s_id, bfa_fcs_lport_get_fcid(port),
				rx_fchs->ox_id, port->port_cfg.roles);

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, NULL, NULL, FC_MAX_PDUSZ, 0);
}

static void
bfa_fcs_rport_process_rpsc(struct bfa_fcs_rport_s *rport,
			struct fchs_s *rx_fchs, u16 len)
{
	struct bfa_fcxp_s *fcxp;
	struct fchs_s	fchs;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fc_rpsc_speed_info_s speeds;
	struct bfa_port_attr_s pport_attr;

	bfa_trc(port->fcs, rx_fchs->s_id);
	bfa_trc(port->fcs, rx_fchs->d_id);

	rport->stats.rpsc_rcvd++;
	speeds.port_speed_cap =
		RPSC_SPEED_CAP_1G | RPSC_SPEED_CAP_2G | RPSC_SPEED_CAP_4G |
		RPSC_SPEED_CAP_8G;

	/*
	 * get curent speed from pport attributes from BFA
	 */
	bfa_fcport_get_attr(port->fcs->bfa, &pport_attr);

	speeds.port_op_speed = fc_bfa_speed_to_rpsc_operspeed(pport_attr.speed);

	fcxp = bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp)
		return;

	len = fc_rpsc_acc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				rx_fchs->s_id, bfa_fcs_lport_get_fcid(port),
				rx_fchs->ox_id, &speeds);

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			FC_CLASS_3, len, &fchs, NULL, NULL, FC_MAX_PDUSZ, 0);
}

static void
bfa_fcs_rport_process_adisc(struct bfa_fcs_rport_s *rport,
			struct fchs_s *rx_fchs, u16 len)
{
	struct bfa_fcxp_s *fcxp;
	struct fchs_s	fchs;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fc_adisc_s	*adisc;

	bfa_trc(port->fcs, rx_fchs->s_id);
	bfa_trc(port->fcs, rx_fchs->d_id);

	rport->stats.adisc_rcvd++;

	adisc = (struct fc_adisc_s *) (rx_fchs + 1);

	/*
	 * Accept if the itnim for this rport is online.
	 * Else reject the ADISC.
	 */
	if (bfa_fcs_itnim_get_online_state(rport->itnim) == BFA_STATUS_OK) {

		fcxp = bfa_fcs_fcxp_alloc(port->fcs);
		if (!fcxp)
			return;

		len = fc_adisc_acc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
			 rx_fchs->s_id, bfa_fcs_lport_get_fcid(port),
			 rx_fchs->ox_id, port->port_cfg.pwwn,
			 port->port_cfg.nwwn);

		bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag,
				BFA_FALSE, FC_CLASS_3, len, &fchs, NULL, NULL,
				FC_MAX_PDUSZ, 0);
	} else {
		rport->stats.adisc_rejected++;
		bfa_fcs_rport_send_ls_rjt(rport, rx_fchs,
					  FC_LS_RJT_RSN_UNABLE_TO_PERF_CMD,
					  FC_LS_RJT_EXP_LOGIN_REQUIRED);
	}
}

static void
bfa_fcs_rport_hal_online(struct bfa_fcs_rport_s *rport)
{
	struct bfa_fcs_lport_s *port = rport->port;
	struct bfa_rport_info_s rport_info;

	rport_info.pid = rport->pid;
	rport_info.local_pid = port->pid;
	rport_info.lp_tag = port->lp_tag;
	rport_info.vf_id = port->fabric->vf_id;
	rport_info.vf_en = port->fabric->is_vf;
	rport_info.fc_class = rport->fc_cos;
	rport_info.cisc = rport->cisc;
	rport_info.max_frmsz = rport->maxfrsize;
	bfa_rport_online(rport->bfa_rport, &rport_info);
}

static struct bfa_fcs_rport_s *
bfa_fcs_rport_alloc(struct bfa_fcs_lport_s *port, wwn_t pwwn, u32 rpid)
{
	struct bfa_fcs_s	*fcs = port->fcs;
	struct bfa_fcs_rport_s *rport;
	struct bfad_rport_s	*rport_drv;

	/**
	 * allocate rport
	 */
	if (bfa_fcb_rport_alloc(fcs->bfad, &rport, &rport_drv)
		!= BFA_STATUS_OK) {
		bfa_trc(fcs, rpid);
		return NULL;
	}

	/*
	 * Initialize r-port
	 */
	rport->port = port;
	rport->fcs = fcs;
	rport->rp_drv = rport_drv;
	rport->pid = rpid;
	rport->pwwn = pwwn;

	/**
	 * allocate BFA rport
	 */
	rport->bfa_rport = bfa_rport_create(port->fcs->bfa, rport);
	if (!rport->bfa_rport) {
		bfa_trc(fcs, rpid);
		kfree(rport_drv);
		return NULL;
	}

	/**
	 * allocate FC-4s
	 */
	bfa_assert(bfa_fcs_lport_is_initiator(port));

	if (bfa_fcs_lport_is_initiator(port)) {
		rport->itnim = bfa_fcs_itnim_create(rport);
		if (!rport->itnim) {
			bfa_trc(fcs, rpid);
			bfa_rport_delete(rport->bfa_rport);
			kfree(rport_drv);
			return NULL;
		}
	}

	bfa_fcs_lport_add_rport(port, rport);

	bfa_sm_set_state(rport, bfa_fcs_rport_sm_uninit);

	/* Initialize the Rport Features(RPF) Sub Module  */
	if (!BFA_FCS_PID_IS_WKA(rport->pid))
		bfa_fcs_rpf_init(rport);

	return rport;
}


static void
bfa_fcs_rport_free(struct bfa_fcs_rport_s *rport)
{
	struct bfa_fcs_lport_s *port = rport->port;

	/**
	 * - delete FC-4s
	 * - delete BFA rport
	 * - remove from queue of rports
	 */
	if (bfa_fcs_lport_is_initiator(port)) {
		bfa_fcs_itnim_delete(rport->itnim);
		if (rport->pid != 0 && !BFA_FCS_PID_IS_WKA(rport->pid))
			bfa_fcs_rpf_rport_offline(rport);
	}

	bfa_rport_delete(rport->bfa_rport);
	bfa_fcs_lport_del_rport(port, rport);
	kfree(rport->rp_drv);
}

static void
bfa_fcs_rport_online_action(struct bfa_fcs_rport_s *rport)
{
	struct bfa_fcs_lport_s *port = rport->port;
	struct bfad_s *bfad = (struct bfad_s *)port->fcs->bfad;
	char	lpwwn_buf[BFA_STRING_32];
	char	rpwwn_buf[BFA_STRING_32];

	rport->stats.onlines++;

	if (bfa_fcs_lport_is_initiator(port)) {
		bfa_fcs_itnim_rport_online(rport->itnim);
		if (!BFA_FCS_PID_IS_WKA(rport->pid))
			bfa_fcs_rpf_rport_online(rport);
	};

	wwn2str(lpwwn_buf, bfa_fcs_lport_get_pwwn(port));
	wwn2str(rpwwn_buf, rport->pwwn);
	if (!BFA_FCS_PID_IS_WKA(rport->pid))
		BFA_LOG(KERN_INFO, bfad, log_level,
		"Remote port (WWN = %s) online for logical port (WWN = %s)\n",
		rpwwn_buf, lpwwn_buf);
}

static void
bfa_fcs_rport_offline_action(struct bfa_fcs_rport_s *rport)
{
	struct bfa_fcs_lport_s *port = rport->port;
	struct bfad_s *bfad = (struct bfad_s *)port->fcs->bfad;
	char	lpwwn_buf[BFA_STRING_32];
	char	rpwwn_buf[BFA_STRING_32];

	rport->stats.offlines++;

	wwn2str(lpwwn_buf, bfa_fcs_lport_get_pwwn(port));
	wwn2str(rpwwn_buf, rport->pwwn);
	if (!BFA_FCS_PID_IS_WKA(rport->pid)) {
		if (bfa_fcs_lport_is_online(rport->port) == BFA_TRUE)
			BFA_LOG(KERN_ERR, bfad, log_level,
				"Remote port (WWN = %s) connectivity lost for "
				"logical port (WWN = %s)\n",
				rpwwn_buf, lpwwn_buf);
		else
			BFA_LOG(KERN_INFO, bfad, log_level,
				"Remote port (WWN = %s) offlined by "
				"logical port (WWN = %s)\n",
				rpwwn_buf, lpwwn_buf);
	}

	if (bfa_fcs_lport_is_initiator(port)) {
		bfa_fcs_itnim_rport_offline(rport->itnim);
		if (!BFA_FCS_PID_IS_WKA(rport->pid))
			bfa_fcs_rpf_rport_offline(rport);
	}
}

/**
 * Update rport parameters from PLOGI or PLOGI accept.
 */
static void
bfa_fcs_rport_update(struct bfa_fcs_rport_s *rport, struct fc_logi_s *plogi)
{
	bfa_fcs_lport_t *port = rport->port;

	/**
	 * - port name
	 * - node name
	 */
	rport->pwwn = plogi->port_name;
	rport->nwwn = plogi->node_name;

	/**
	 * - class of service
	 */
	rport->fc_cos = 0;
	if (plogi->class3.class_valid)
		rport->fc_cos = FC_CLASS_3;

	if (plogi->class2.class_valid)
		rport->fc_cos |= FC_CLASS_2;

	/**
	 * - CISC
	 * - MAX receive frame size
	 */
	rport->cisc = plogi->csp.cisc;
	rport->maxfrsize = be16_to_cpu(plogi->class3.rxsz);

	bfa_trc(port->fcs, be16_to_cpu(plogi->csp.bbcred));
	bfa_trc(port->fcs, port->fabric->bb_credit);
	/**
	 * Direct Attach P2P mode :
	 * This is to handle a bug (233476) in IBM targets in Direct Attach
	 *  Mode. Basically, in FLOGI Accept the target would have
	 * erroneously set the BB Credit to the value used in the FLOGI
	 * sent by the HBA. It uses the correct value (its own BB credit)
	 * in PLOGI.
	 */
	if ((!bfa_fcs_fabric_is_switched(port->fabric))	 &&
		(be16_to_cpu(plogi->csp.bbcred) < port->fabric->bb_credit)) {

		bfa_trc(port->fcs, be16_to_cpu(plogi->csp.bbcred));
		bfa_trc(port->fcs, port->fabric->bb_credit);

		port->fabric->bb_credit = be16_to_cpu(plogi->csp.bbcred);
		bfa_fcport_set_tx_bbcredit(port->fcs->bfa,
					  port->fabric->bb_credit);
	}

}

/**
 *	Called to handle LOGO received from an existing remote port.
 */
static void
bfa_fcs_rport_process_logo(struct bfa_fcs_rport_s *rport, struct fchs_s *fchs)
{
	rport->reply_oxid = fchs->ox_id;
	bfa_trc(rport->fcs, rport->reply_oxid);

	rport->prlo = BFA_FALSE;
	rport->stats.logo_rcvd++;
	bfa_sm_send_event(rport, RPSM_EVENT_LOGO_RCVD);
}



/**
 *  fcs_rport_public FCS rport public interfaces
 */

/**
 *	Called by bport/vport to create a remote port instance for a discovered
 *	remote device.
 *
 * @param[in] port	- base port or vport
 * @param[in] rpid	- remote port ID
 *
 * @return None
 */
struct bfa_fcs_rport_s *
bfa_fcs_rport_create(struct bfa_fcs_lport_s *port, u32 rpid)
{
	struct bfa_fcs_rport_s *rport;

	bfa_trc(port->fcs, rpid);
	rport = bfa_fcs_rport_alloc(port, WWN_NULL, rpid);
	if (!rport)
		return NULL;

	bfa_sm_send_event(rport, RPSM_EVENT_PLOGI_SEND);
	return rport;
}

/**
 * Called to create a rport for which only the wwn is known.
 *
 * @param[in] port	- base port
 * @param[in] rpwwn	- remote port wwn
 *
 * @return None
 */
struct bfa_fcs_rport_s *
bfa_fcs_rport_create_by_wwn(struct bfa_fcs_lport_s *port, wwn_t rpwwn)
{
	struct bfa_fcs_rport_s *rport;
	bfa_trc(port->fcs, rpwwn);
	rport = bfa_fcs_rport_alloc(port, rpwwn, 0);
	if (!rport)
		return NULL;

	bfa_sm_send_event(rport, RPSM_EVENT_ADDRESS_DISC);
	return rport;
}
/**
 * Called by bport in private loop topology to indicate that a
 * rport has been discovered and plogi has been completed.
 *
 * @param[in] port	- base port or vport
 * @param[in] rpid	- remote port ID
 */
void
bfa_fcs_rport_start(struct bfa_fcs_lport_s *port, struct fchs_s *fchs,
	 struct fc_logi_s *plogi)
{
	struct bfa_fcs_rport_s *rport;

	rport = bfa_fcs_rport_alloc(port, WWN_NULL, fchs->s_id);
	if (!rport)
		return;

	bfa_fcs_rport_update(rport, plogi);

	bfa_sm_send_event(rport, RPSM_EVENT_PLOGI_COMP);
}

/**
 *	Called by bport/vport to handle PLOGI received from a new remote port.
 *	If an existing rport does a plogi, it will be handled separately.
 */
void
bfa_fcs_rport_plogi_create(struct bfa_fcs_lport_s *port, struct fchs_s *fchs,
				struct fc_logi_s *plogi)
{
	struct bfa_fcs_rport_s *rport;

	rport = bfa_fcs_rport_alloc(port, plogi->port_name, fchs->s_id);
	if (!rport)
		return;

	bfa_fcs_rport_update(rport, plogi);

	rport->reply_oxid = fchs->ox_id;
	bfa_trc(rport->fcs, rport->reply_oxid);

	rport->stats.plogi_rcvd++;
	bfa_sm_send_event(rport, RPSM_EVENT_PLOGI_RCVD);
}

static int
wwn_compare(wwn_t wwn1, wwn_t wwn2)
{
	u8		*b1 = (u8 *) &wwn1;
	u8		*b2 = (u8 *) &wwn2;
	int		i;

	for (i = 0; i < sizeof(wwn_t); i++) {
		if (b1[i] < b2[i])
			return -1;
		if (b1[i] > b2[i])
			return 1;
	}
	return 0;
}

/**
 *	Called by bport/vport to handle PLOGI received from an existing
 *	 remote port.
 */
void
bfa_fcs_rport_plogi(struct bfa_fcs_rport_s *rport, struct fchs_s *rx_fchs,
			struct fc_logi_s *plogi)
{
	/**
	 * @todo Handle P2P and initiator-initiator.
	 */

	bfa_fcs_rport_update(rport, plogi);

	rport->reply_oxid = rx_fchs->ox_id;
	bfa_trc(rport->fcs, rport->reply_oxid);

	/**
	 * In Switched fabric topology,
	 * PLOGI to each other. If our pwwn is smaller, ignore it,
	 * if it is not a well known address.
	 * If the link topology is N2N,
	 * this Plogi should be accepted.
	 */
	if ((wwn_compare(rport->port->port_cfg.pwwn, rport->pwwn) == -1) &&
		(bfa_fcs_fabric_is_switched(rport->port->fabric)) &&
		(!BFA_FCS_PID_IS_WKA(rport->pid))) {
		bfa_trc(rport->fcs, rport->pid);
		return;
	}

	rport->stats.plogi_rcvd++;
	bfa_sm_send_event(rport, RPSM_EVENT_PLOGI_RCVD);
}

/**
 * Called by bport/vport to delete a remote port instance.
 *
 * Rport delete is called under the following conditions:
 *		- vport is deleted
 *		- vf is deleted
 *		- explicit request from OS to delete rport
 */
void
bfa_fcs_rport_delete(struct bfa_fcs_rport_s *rport)
{
	bfa_sm_send_event(rport, RPSM_EVENT_DELETE);
}

/**
 * Called by bport/vport to  when a target goes offline.
 *
 */
void
bfa_fcs_rport_offline(struct bfa_fcs_rport_s *rport)
{
	bfa_sm_send_event(rport, RPSM_EVENT_LOGO_IMP);
}

/**
 * Called by bport in n2n when a target (attached port) becomes online.
 *
 */
void
bfa_fcs_rport_online(struct bfa_fcs_rport_s *rport)
{
	bfa_sm_send_event(rport, RPSM_EVENT_PLOGI_SEND);
}
/**
 *	Called by bport/vport to notify SCN for the remote port
 */
void
bfa_fcs_rport_scn(struct bfa_fcs_rport_s *rport)
{
	rport->stats.rscns++;
	bfa_sm_send_event(rport, RPSM_EVENT_SCN);
}

/**
 *	Called by	fcpim to notify that the ITN cleanup is done.
 */
void
bfa_fcs_rport_itnim_ack(struct bfa_fcs_rport_s *rport)
{
	bfa_sm_send_event(rport, RPSM_EVENT_FC4_OFFLINE);
}

/**
 *	Called by fcptm to notify that the ITN cleanup is done.
 */
void
bfa_fcs_rport_tin_ack(struct bfa_fcs_rport_s *rport)
{
	bfa_sm_send_event(rport, RPSM_EVENT_FC4_OFFLINE);
}

/**
 *	brief
 *	This routine BFA callback for bfa_rport_online() call.
 *
 *	param[in]	cb_arg	-  rport struct.
 *
 *	return
 *		void
 *
 *	Special Considerations:
 *
 *	note
 */
void
bfa_cb_rport_online(void *cbarg)
{

	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;

	bfa_trc(rport->fcs, rport->pwwn);
	bfa_sm_send_event(rport, RPSM_EVENT_HCB_ONLINE);
}

/**
 *	brief
 *	This routine BFA callback for bfa_rport_offline() call.
 *
 *	param[in]	rport	-
 *
 *	return
 *		void
 *
 *	Special Considerations:
 *
 *	note
 */
void
bfa_cb_rport_offline(void *cbarg)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;

	bfa_trc(rport->fcs, rport->pwwn);
	bfa_sm_send_event(rport, RPSM_EVENT_HCB_OFFLINE);
}

/**
 *	brief
 *	This routine is a static BFA callback when there is a QoS flow_id
 *	change notification
 *
 *	param[in]	rport	-
 *
 *	return
 *		void
 *
 *	Special Considerations:
 *
 *	note
 */
void
bfa_cb_rport_qos_scn_flowid(void *cbarg,
		struct bfa_rport_qos_attr_s old_qos_attr,
		struct bfa_rport_qos_attr_s new_qos_attr)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;

	bfa_trc(rport->fcs, rport->pwwn);
}

/**
 *	brief
 *	This routine is a static BFA callback when there is a QoS priority
 *	change notification
 *
 *	param[in]	rport	-
 *
 *	return
 *		void
 *
 *	Special Considerations:
 *
 *	note
 */
void
bfa_cb_rport_qos_scn_prio(void *cbarg,
		struct bfa_rport_qos_attr_s old_qos_attr,
		struct bfa_rport_qos_attr_s new_qos_attr)
{
	struct bfa_fcs_rport_s *rport = (struct bfa_fcs_rport_s *) cbarg;

	bfa_trc(rport->fcs, rport->pwwn);
}

/**
 *		Called to process any unsolicted frames from this remote port
 */
void
bfa_fcs_rport_logo_imp(struct bfa_fcs_rport_s *rport)
{
	bfa_sm_send_event(rport, RPSM_EVENT_LOGO_IMP);
}

/**
 *		Called to process any unsolicted frames from this remote port
 */
void
bfa_fcs_rport_uf_recv(struct bfa_fcs_rport_s *rport,
			struct fchs_s *fchs, u16 len)
{
	struct bfa_fcs_lport_s *port = rport->port;
	struct fc_els_cmd_s	*els_cmd;

	bfa_trc(rport->fcs, fchs->s_id);
	bfa_trc(rport->fcs, fchs->d_id);
	bfa_trc(rport->fcs, fchs->type);

	if (fchs->type != FC_TYPE_ELS)
		return;

	els_cmd = (struct fc_els_cmd_s *) (fchs + 1);

	bfa_trc(rport->fcs, els_cmd->els_code);

	switch (els_cmd->els_code) {
	case FC_ELS_LOGO:
		bfa_stats(port, plogi_rcvd);
		bfa_fcs_rport_process_logo(rport, fchs);
		break;

	case FC_ELS_ADISC:
		bfa_stats(port, adisc_rcvd);
		bfa_fcs_rport_process_adisc(rport, fchs, len);
		break;

	case FC_ELS_PRLO:
		bfa_stats(port, prlo_rcvd);
		if (bfa_fcs_lport_is_initiator(port))
			bfa_fcs_fcpim_uf_recv(rport->itnim, fchs, len);
		break;

	case FC_ELS_PRLI:
		bfa_stats(port, prli_rcvd);
		bfa_fcs_rport_process_prli(rport, fchs, len);
		break;

	case FC_ELS_RPSC:
		bfa_stats(port, rpsc_rcvd);
		bfa_fcs_rport_process_rpsc(rport, fchs, len);
		break;

	default:
		bfa_stats(port, un_handled_els_rcvd);
		bfa_fcs_rport_send_ls_rjt(rport, fchs,
					  FC_LS_RJT_RSN_CMD_NOT_SUPP,
					  FC_LS_RJT_EXP_NO_ADDL_INFO);
		break;
	}
}

/* send best case  acc to prlo */
static void
bfa_fcs_rport_send_prlo_acc(struct bfa_fcs_rport_s *rport)
{
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s	fchs;
	struct bfa_fcxp_s *fcxp;
	int		len;

	bfa_trc(rport->fcs, rport->pid);

	fcxp = bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp)
		return;
	len = fc_prlo_acc_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
			rport->pid, bfa_fcs_lport_get_fcid(port),
			rport->reply_oxid, 0);

	bfa_fcxp_send(fcxp, rport->bfa_rport, port->fabric->vf_id,
		port->lp_tag, BFA_FALSE, FC_CLASS_3, len, &fchs,
		NULL, NULL, FC_MAX_PDUSZ, 0);
}

/*
 * Send a LS reject
 */
static void
bfa_fcs_rport_send_ls_rjt(struct bfa_fcs_rport_s *rport, struct fchs_s *rx_fchs,
			  u8 reason_code, u8 reason_code_expl)
{
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s	fchs;
	struct bfa_fcxp_s *fcxp;
	int		len;

	bfa_trc(rport->fcs, rx_fchs->s_id);

	fcxp = bfa_fcs_fcxp_alloc(rport->fcs);
	if (!fcxp)
		return;

	len = fc_ls_rjt_build(&fchs, bfa_fcxp_get_reqbuf(fcxp),
				rx_fchs->s_id, bfa_fcs_lport_get_fcid(port),
				rx_fchs->ox_id, reason_code, reason_code_expl);

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag,
			BFA_FALSE, FC_CLASS_3, len, &fchs, NULL, NULL,
			FC_MAX_PDUSZ, 0);
}

/**
 * Return state of rport.
 */
int
bfa_fcs_rport_get_state(struct bfa_fcs_rport_s *rport)
{
	return bfa_sm_to_state(rport_sm_table, rport->sm);
}

/**
 *	brief
 *		 Called by the Driver to set rport delete/ageout timeout
 *
 *	param[in]		rport timeout value in seconds.
 *
 *	return None
 */
void
bfa_fcs_rport_set_del_timeout(u8 rport_tmo)
{
	/* convert to Millisecs */
	if (rport_tmo > 0)
		bfa_fcs_rport_del_timeout = rport_tmo * 1000;
}
void
bfa_fcs_rport_prlo(struct bfa_fcs_rport_s *rport, u16 ox_id)
{
	bfa_trc(rport->fcs, rport->pid);

	rport->prlo = BFA_TRUE;
	rport->reply_oxid = ox_id;
	bfa_sm_send_event(rport, RPSM_EVENT_PRLO_RCVD);
}



/**
 * Remote port implementation.
 */

/**
 *  fcs_rport_api FCS rport API.
 */

/**
 *	Direct API to add a target by port wwn. This interface is used, for
 *	example, by bios when target pwwn is known from boot lun configuration.
 */
bfa_status_t
bfa_fcs_rport_add(struct bfa_fcs_lport_s *port, wwn_t *pwwn,
		struct bfa_fcs_rport_s *rport, struct bfad_rport_s *rport_drv)
{
	bfa_trc(port->fcs, *pwwn);

	return BFA_STATUS_OK;
}

/**
 *	Direct API to remove a target and its associated resources. This
 *	interface is used, for example, by driver to remove target
 *	ports from the target list for a VM.
 */
bfa_status_t
bfa_fcs_rport_remove(struct bfa_fcs_rport_s *rport_in)
{

	struct bfa_fcs_rport_s *rport;

	bfa_trc(rport_in->fcs, rport_in->pwwn);

	rport = bfa_fcs_lport_get_rport_by_pwwn(rport_in->port, rport_in->pwwn);
	if (rport == NULL) {
		/*
		 * TBD Error handling
		 */
		bfa_trc(rport_in->fcs, rport_in->pid);
		return BFA_STATUS_UNKNOWN_RWWN;
	}

	/*
	 * TBD if this remote port is online, send a logo
	 */
	return BFA_STATUS_OK;

}

/**
 *	Remote device status for display/debug.
 */
void
bfa_fcs_rport_get_attr(struct bfa_fcs_rport_s *rport,
			struct bfa_rport_attr_s *rport_attr)
{
	struct bfa_rport_qos_attr_s qos_attr;
	bfa_fcs_lport_t *port = rport->port;
	bfa_port_speed_t rport_speed = rport->rpf.rpsc_speed;

	memset(rport_attr, 0, sizeof(struct bfa_rport_attr_s));

	rport_attr->pid = rport->pid;
	rport_attr->pwwn = rport->pwwn;
	rport_attr->nwwn = rport->nwwn;
	rport_attr->cos_supported = rport->fc_cos;
	rport_attr->df_sz = rport->maxfrsize;
	rport_attr->state = bfa_fcs_rport_get_state(rport);
	rport_attr->fc_cos = rport->fc_cos;
	rport_attr->cisc = rport->cisc;
	rport_attr->scsi_function = rport->scsi_function;
	rport_attr->curr_speed  = rport->rpf.rpsc_speed;
	rport_attr->assigned_speed  = rport->rpf.assigned_speed;

	bfa_rport_get_qos_attr(rport->bfa_rport, &qos_attr);
	rport_attr->qos_attr = qos_attr;

	rport_attr->trl_enforced = BFA_FALSE;
	if (bfa_fcport_is_ratelim(port->fcs->bfa)) {
		if (rport_speed == BFA_PORT_SPEED_UNKNOWN) {
			/* Use default ratelim speed setting */
			rport_speed =
				bfa_fcport_get_ratelim_speed(rport->fcs->bfa);
		}

		if (rport_speed < bfa_fcs_lport_get_rport_max_speed(port))
			rport_attr->trl_enforced = BFA_TRUE;
	}
}

/**
 *	Per remote device statistics.
 */
void
bfa_fcs_rport_get_stats(struct bfa_fcs_rport_s *rport,
			struct bfa_rport_stats_s *stats)
{
	*stats = rport->stats;
}

void
bfa_fcs_rport_clear_stats(struct bfa_fcs_rport_s *rport)
{
	memset((char *)&rport->stats, 0,
			sizeof(struct bfa_rport_stats_s));
}

struct bfa_fcs_rport_s *
bfa_fcs_rport_lookup(struct bfa_fcs_lport_s *port, wwn_t rpwwn)
{
	struct bfa_fcs_rport_s *rport;

	rport = bfa_fcs_lport_get_rport_by_pwwn(port, rpwwn);
	if (rport == NULL) {
		/*
		 * TBD Error handling
		 */
	}

	return rport;
}

struct bfa_fcs_rport_s *
bfa_fcs_rport_lookup_by_nwwn(struct bfa_fcs_lport_s *port, wwn_t rnwwn)
{
	struct bfa_fcs_rport_s *rport;

	rport = bfa_fcs_lport_get_rport_by_nwwn(port, rnwwn);
	if (rport == NULL) {
		/*
		 * TBD Error handling
		 */
	}

	return rport;
}

/*
 * This API is to set the Rport's speed. Should be used when RPSC is not
 * supported by the rport.
 */
void
bfa_fcs_rport_set_speed(struct bfa_fcs_rport_s *rport, bfa_port_speed_t speed)
{
	rport->rpf.assigned_speed  = speed;

	/* Set this speed in f/w only if the RPSC speed is not available */
	if (rport->rpf.rpsc_speed == BFA_PORT_SPEED_UNKNOWN)
		bfa_rport_speed(rport->bfa_rport, speed);
}



/**
 * Remote port features (RPF) implementation.
 */

#define BFA_FCS_RPF_RETRIES	(3)
#define BFA_FCS_RPF_RETRY_TIMEOUT  (1000) /* 1 sec (In millisecs) */

static void     bfa_fcs_rpf_send_rpsc2(void *rport_cbarg,
				struct bfa_fcxp_s *fcxp_alloced);
static void     bfa_fcs_rpf_rpsc2_response(void *fcsarg,
			struct bfa_fcxp_s *fcxp,
			void *cbarg,
			bfa_status_t req_status,
			u32 rsp_len,
			u32 resid_len,
			struct fchs_s *rsp_fchs);

static void     bfa_fcs_rpf_timeout(void *arg);

/**
 *  fcs_rport_ftrs_sm FCS rport state machine events
 */

enum rpf_event {
	RPFSM_EVENT_RPORT_OFFLINE  = 1, /* Rport offline		*/
	RPFSM_EVENT_RPORT_ONLINE   = 2,	/* Rport online			*/
	RPFSM_EVENT_FCXP_SENT      = 3,	/* Frame from has been sent	*/
	RPFSM_EVENT_TIMEOUT	   = 4, /* Rport SM timeout event	*/
	RPFSM_EVENT_RPSC_COMP      = 5,
	RPFSM_EVENT_RPSC_FAIL      = 6,
	RPFSM_EVENT_RPSC_ERROR     = 7,
};

static void	bfa_fcs_rpf_sm_uninit(struct bfa_fcs_rpf_s *rpf,
					enum rpf_event event);
static void     bfa_fcs_rpf_sm_rpsc_sending(struct bfa_fcs_rpf_s *rpf,
				       enum rpf_event event);
static void     bfa_fcs_rpf_sm_rpsc(struct bfa_fcs_rpf_s *rpf,
				       enum rpf_event event);
static void	bfa_fcs_rpf_sm_rpsc_retry(struct bfa_fcs_rpf_s *rpf,
					enum rpf_event event);
static void     bfa_fcs_rpf_sm_offline(struct bfa_fcs_rpf_s *rpf,
					enum rpf_event event);
static void     bfa_fcs_rpf_sm_online(struct bfa_fcs_rpf_s *rpf,
					enum rpf_event event);

static void
bfa_fcs_rpf_sm_uninit(struct bfa_fcs_rpf_s *rpf, enum rpf_event event)
{
	struct bfa_fcs_rport_s *rport = rpf->rport;
	struct bfa_fcs_fabric_s *fabric = &rport->fcs->fabric;

	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPFSM_EVENT_RPORT_ONLINE:
		/* Send RPSC2 to a Brocade fabric only. */
		if ((!BFA_FCS_PID_IS_WKA(rport->pid)) &&
			((bfa_lps_is_brcd_fabric(rport->port->fabric->lps)) ||
			(bfa_fcs_fabric_get_switch_oui(fabric) ==
						BFA_FCS_BRCD_SWITCH_OUI))) {
			bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_rpsc_sending);
			rpf->rpsc_retries = 0;
			bfa_fcs_rpf_send_rpsc2(rpf, NULL);
		}
		break;

	case RPFSM_EVENT_RPORT_OFFLINE:
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

static void
bfa_fcs_rpf_sm_rpsc_sending(struct bfa_fcs_rpf_s *rpf, enum rpf_event event)
{
	struct bfa_fcs_rport_s *rport = rpf->rport;

	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPFSM_EVENT_FCXP_SENT:
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_rpsc);
		break;

	case RPFSM_EVENT_RPORT_OFFLINE:
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_offline);
		bfa_fcxp_walloc_cancel(rport->fcs->bfa, &rpf->fcxp_wqe);
		rpf->rpsc_retries = 0;
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

static void
bfa_fcs_rpf_sm_rpsc(struct bfa_fcs_rpf_s *rpf, enum rpf_event event)
{
	struct bfa_fcs_rport_s *rport = rpf->rport;

	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPFSM_EVENT_RPSC_COMP:
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_online);
		/* Update speed info in f/w via BFA */
		if (rpf->rpsc_speed != BFA_PORT_SPEED_UNKNOWN)
			bfa_rport_speed(rport->bfa_rport, rpf->rpsc_speed);
		else if (rpf->assigned_speed != BFA_PORT_SPEED_UNKNOWN)
			bfa_rport_speed(rport->bfa_rport, rpf->assigned_speed);
		break;

	case RPFSM_EVENT_RPSC_FAIL:
		/* RPSC not supported by rport */
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_online);
		break;

	case RPFSM_EVENT_RPSC_ERROR:
		/* need to retry...delayed a bit. */
		if (rpf->rpsc_retries++ < BFA_FCS_RPF_RETRIES) {
			bfa_timer_start(rport->fcs->bfa, &rpf->timer,
				    bfa_fcs_rpf_timeout, rpf,
				    BFA_FCS_RPF_RETRY_TIMEOUT);
			bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_rpsc_retry);
		} else {
			bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_online);
		}
		break;

	case RPFSM_EVENT_RPORT_OFFLINE:
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_offline);
		bfa_fcxp_discard(rpf->fcxp);
		rpf->rpsc_retries = 0;
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

static void
bfa_fcs_rpf_sm_rpsc_retry(struct bfa_fcs_rpf_s *rpf, enum rpf_event event)
{
	struct bfa_fcs_rport_s *rport = rpf->rport;

	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPFSM_EVENT_TIMEOUT:
		/* re-send the RPSC */
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_rpsc_sending);
		bfa_fcs_rpf_send_rpsc2(rpf, NULL);
		break;

	case RPFSM_EVENT_RPORT_OFFLINE:
		bfa_timer_stop(&rpf->timer);
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_offline);
		rpf->rpsc_retries = 0;
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

static void
bfa_fcs_rpf_sm_online(struct bfa_fcs_rpf_s *rpf, enum rpf_event event)
{
	struct bfa_fcs_rport_s *rport = rpf->rport;

	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPFSM_EVENT_RPORT_OFFLINE:
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_offline);
		rpf->rpsc_retries = 0;
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}

static void
bfa_fcs_rpf_sm_offline(struct bfa_fcs_rpf_s *rpf, enum rpf_event event)
{
	struct bfa_fcs_rport_s *rport = rpf->rport;

	bfa_trc(rport->fcs, rport->pwwn);
	bfa_trc(rport->fcs, rport->pid);
	bfa_trc(rport->fcs, event);

	switch (event) {
	case RPFSM_EVENT_RPORT_ONLINE:
		bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_rpsc_sending);
		bfa_fcs_rpf_send_rpsc2(rpf, NULL);
		break;

	case RPFSM_EVENT_RPORT_OFFLINE:
		break;

	default:
		bfa_sm_fault(rport->fcs, event);
	}
}
/**
 * Called when Rport is created.
 */
void
bfa_fcs_rpf_init(struct bfa_fcs_rport_s *rport)
{
	struct bfa_fcs_rpf_s *rpf = &rport->rpf;

	bfa_trc(rport->fcs, rport->pid);
	rpf->rport = rport;

	bfa_sm_set_state(rpf, bfa_fcs_rpf_sm_uninit);
}

/**
 * Called when Rport becomes online
 */
void
bfa_fcs_rpf_rport_online(struct bfa_fcs_rport_s *rport)
{
	bfa_trc(rport->fcs, rport->pid);

	if (__fcs_min_cfg(rport->port->fcs))
		return;

	if (bfa_fcs_fabric_is_switched(rport->port->fabric))
		bfa_sm_send_event(&rport->rpf, RPFSM_EVENT_RPORT_ONLINE);
}

/**
 * Called when Rport becomes offline
 */
void
bfa_fcs_rpf_rport_offline(struct bfa_fcs_rport_s *rport)
{
	bfa_trc(rport->fcs, rport->pid);

	if (__fcs_min_cfg(rport->port->fcs))
		return;

	rport->rpf.rpsc_speed = 0;
	bfa_sm_send_event(&rport->rpf, RPFSM_EVENT_RPORT_OFFLINE);
}

static void
bfa_fcs_rpf_timeout(void *arg)
{
	struct bfa_fcs_rpf_s *rpf = (struct bfa_fcs_rpf_s *) arg;
	struct bfa_fcs_rport_s *rport = rpf->rport;

	bfa_trc(rport->fcs, rport->pid);
	bfa_sm_send_event(rpf, RPFSM_EVENT_TIMEOUT);
}

static void
bfa_fcs_rpf_send_rpsc2(void *rpf_cbarg, struct bfa_fcxp_s *fcxp_alloced)
{
	struct bfa_fcs_rpf_s *rpf = (struct bfa_fcs_rpf_s *)rpf_cbarg;
	struct bfa_fcs_rport_s *rport = rpf->rport;
	struct bfa_fcs_lport_s *port = rport->port;
	struct fchs_s	fchs;
	int		len;
	struct bfa_fcxp_s *fcxp;

	bfa_trc(rport->fcs, rport->pwwn);

	fcxp = fcxp_alloced ? fcxp_alloced : bfa_fcs_fcxp_alloc(port->fcs);
	if (!fcxp) {
		bfa_fcs_fcxp_alloc_wait(port->fcs->bfa, &rpf->fcxp_wqe,
					bfa_fcs_rpf_send_rpsc2, rpf);
		return;
	}
	rpf->fcxp = fcxp;

	len = fc_rpsc2_build(&fchs, bfa_fcxp_get_reqbuf(fcxp), rport->pid,
			    bfa_fcs_lport_get_fcid(port), &rport->pid, 1);

	bfa_fcxp_send(fcxp, NULL, port->fabric->vf_id, port->lp_tag, BFA_FALSE,
			  FC_CLASS_3, len, &fchs, bfa_fcs_rpf_rpsc2_response,
			  rpf, FC_MAX_PDUSZ, FC_ELS_TOV);
	rport->stats.rpsc_sent++;
	bfa_sm_send_event(rpf, RPFSM_EVENT_FCXP_SENT);

}

static void
bfa_fcs_rpf_rpsc2_response(void *fcsarg, struct bfa_fcxp_s *fcxp, void *cbarg,
			    bfa_status_t req_status, u32 rsp_len,
			    u32 resid_len, struct fchs_s *rsp_fchs)
{
	struct bfa_fcs_rpf_s *rpf = (struct bfa_fcs_rpf_s *) cbarg;
	struct bfa_fcs_rport_s *rport = rpf->rport;
	struct fc_ls_rjt_s *ls_rjt;
	struct fc_rpsc2_acc_s *rpsc2_acc;
	u16	num_ents;

	bfa_trc(rport->fcs, req_status);

	if (req_status != BFA_STATUS_OK) {
		bfa_trc(rport->fcs, req_status);
		if (req_status == BFA_STATUS_ETIMER)
			rport->stats.rpsc_failed++;
		bfa_sm_send_event(rpf, RPFSM_EVENT_RPSC_ERROR);
		return;
	}

	rpsc2_acc = (struct fc_rpsc2_acc_s *) BFA_FCXP_RSP_PLD(fcxp);
	if (rpsc2_acc->els_cmd == FC_ELS_ACC) {
		rport->stats.rpsc_accs++;
		num_ents = be16_to_cpu(rpsc2_acc->num_pids);
		bfa_trc(rport->fcs, num_ents);
		if (num_ents > 0) {
			bfa_assert(rpsc2_acc->port_info[0].pid != rport->pid);
			bfa_trc(rport->fcs,
				be16_to_cpu(rpsc2_acc->port_info[0].pid));
			bfa_trc(rport->fcs,
				be16_to_cpu(rpsc2_acc->port_info[0].speed));
			bfa_trc(rport->fcs,
				be16_to_cpu(rpsc2_acc->port_info[0].index));
			bfa_trc(rport->fcs,
				rpsc2_acc->port_info[0].type);

			if (rpsc2_acc->port_info[0].speed == 0) {
				bfa_sm_send_event(rpf, RPFSM_EVENT_RPSC_ERROR);
				return;
			}

			rpf->rpsc_speed = fc_rpsc_operspeed_to_bfa_speed(
				be16_to_cpu(rpsc2_acc->port_info[0].speed));

			bfa_sm_send_event(rpf, RPFSM_EVENT_RPSC_COMP);
		}
	} else {
		ls_rjt = (struct fc_ls_rjt_s *) BFA_FCXP_RSP_PLD(fcxp);
		bfa_trc(rport->fcs, ls_rjt->reason_code);
		bfa_trc(rport->fcs, ls_rjt->reason_code_expl);
		rport->stats.rpsc_rejects++;
		if (ls_rjt->reason_code == FC_LS_RJT_RSN_CMD_NOT_SUPP)
			bfa_sm_send_event(rpf, RPFSM_EVENT_RPSC_FAIL);
		else
			bfa_sm_send_event(rpf, RPFSM_EVENT_RPSC_ERROR);
	}
}
