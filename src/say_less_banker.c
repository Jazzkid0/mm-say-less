#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "attributes.h"

#include "overlays/actors/ovl_En_Ginko_Man/z_en_ginko_man.h"

extern enum {
    /* 0 */ GINKO_ANIM_LEGSMACKING,
    /* 1 */ GINKO_ANIM_SITTING,
    /* 2 */ GINKO_ANIM_REACHING,
    /* 3 */ GINKO_ANIM_AMAZED,
    /* 4 */ GINKO_ANIM_ADVERTISING, // looking around for customers
    /* 5 */ GINKO_ANIM_MAX
} GinkoAnimation;

static AnimationInfo sAnimationInfo[GINKO_ANIM_MAX] = {
    { &object_boj_Anim_0008C0, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_LEGSMACKING
    { &object_boj_Anim_0043F0, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_SITTING
    { &object_boj_Anim_004F40, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -4.0f }, // GINKO_ANIM_REACHING
    { &object_boj_Anim_000AC4, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_AMAZED
    { &object_boj_Anim_004A7C, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -4.0f }, // GINKO_ANIM_ADVERTISING
};

extern void EnGinkoMan_Init(Actor* thisx, PlayState* play);
extern void EnGinkoMan_Destroy(Actor* thisx, PlayState* play);
extern void EnGinkoMan_Update(Actor* thisx, PlayState* play);
extern void EnGinkoMan_Draw(Actor* thisx, PlayState* play);

extern void EnGinkoMan_SetupIdle(EnGinkoMan* this);
extern void EnGinkoMan_SetupDialogue(EnGinkoMan* this);
extern void EnGinkoMan_SetupBankAward(EnGinkoMan* this);
extern void EnGinkoMan_SetupBankAward2(EnGinkoMan* this);
extern void EnGinkoMan_SetupStamp(EnGinkoMan* this);

extern void EnGinkoMan_BankAward(EnGinkoMan* this, PlayState* play);
extern void EnGinkoMan_BankAward2(EnGinkoMan* this, PlayState* play);
extern void EnGinkoMan_Dialogue(EnGinkoMan* this, PlayState* play);
extern void EnGinkoMan_SwitchAnimation(EnGinkoMan* this, PlayState* play);

RECOMP_PATCH void EnGinkoMan_Idle(EnGinkoMan* this, PlayState* play) {
    s32 yaw = this->actor.yawTowardsPlayer - this->actor.shape.rot.y;

    EnGinkoMan_SwitchAnimation(this, play);
    if (Actor_TalkOfferAccepted(&this->actor, &play->state)) {
        if (HS_GET_BANK_RUPEES() == 0) {
            if (CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_BANK_WALLET_UPGRADE)) {
                Message_StartTextbox(play, 0x44E, &this->actor);
                this->curTextId = 0x44E; // deposit or exit
            } else {
                Message_StartTextbox(play, 0x44D, &this->actor);
                this->curTextId = 0x44D; // example deposit reward. Leave in for rando to spoil item?
            }
        } else {
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
            Message_StartTextbox(play, 0x468, &this->actor);
            this->curTextId = 0x468; // deposit, withdraw or exit
        }
        EnGinkoMan_SetupDialogue(this);
    } else if (ABS_ALT(yaw) < 0x1555) {
        Actor_OfferTalk(&this->actor, play, 100.0f);
    }
}

RECOMP_PATCH void EnGinkoMan_DepositDialogue(EnGinkoMan* this, PlayState* play) {
    if (!Message_ShouldAdvance(play)) {
        return;
    }

    switch (this->curTextId) {
        case 0x44C: // initial greeting (removed)
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
            if (CHECK_WEEKEVENTREG(WEEKEVENTREG_RECEIVED_BANK_WALLET_UPGRADE)) {
                Message_StartTextbox(play, 0x44E, &this->actor);
                this->curTextId = 0x44E; // deposit or exit
            } else {
                Message_StartTextbox(play, 0x44D, &this->actor);
                this->curTextId = 0x44D; // example deposit reward. Leave in for rando to spoil item?
            }
            break;

        case 0x44D: // example deposit reward (left in for rando)
            Message_StartTextbox(play, 0x44E, &this->actor);
            this->curTextId = 0x44E; // deposit or exit
            break;

        case 0x44F: // All right! So... (removed)
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            Message_StartTextbox(play, 0x450, &this->actor);
            this->curTextId = 0x450; // deposit how much [prompt]
            break;

        case 0x453: // you deposited a tiny amount (removed)
        case 0x454: // you deposited a normal amount (removed)
        case 0x455: // you deposited a lot (removed)
            if (this->curTextId == 0x453) { // tiny amount == smack legs
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            }

            play->msgCtx.rupeesTotal = HS_GET_BANK_RUPEES();
            Message_StartTextbox(play, 0x45A, &this->actor);
            this->curTextId = 0x45A; // show rupee total
            break;

        case 0x456: // cancel deposit (account empty)
        case 0x459: // you don't have that much in wallet
            Message_StartTextbox(play, 0x44E, &this->actor);
            this->curTextId = 0x44E; // deposit or exit
            break;

        case 0x45A: // show rupee total
            if ((HS_GET_BANK_RUPEES() >= 200) && (this->previousBankValue < 200) &&
                !CHECK_WEEKEVENTREG(WEEKEVENTREG_59_40)) {
                SET_WEEKEVENTREG(WEEKEVENTREG_59_40);
                Message_StartTextbox(play, 0x45B, &this->actor);
                this->curTextId = 0x45B; // exceeded 200
            } else if ((HS_GET_BANK_RUPEES() >= 1000) && (this->previousBankValue < 1000) &&
                       !CHECK_WEEKEVENTREG(WEEKEVENTREG_59_80)) {
                SET_WEEKEVENTREG(WEEKEVENTREG_59_80);
                Message_StartTextbox(play, 0x45C, &this->actor);
                this->curTextId = 0x45C; // exceeded 1000
            } else if (HS_GET_BANK_RUPEES() >= 5000) {
                if ((this->previousBankValue < 5000) && !CHECK_WEEKEVENTREG(WEEKEVENTREG_60_01)) {
                    SET_WEEKEVENTREG(WEEKEVENTREG_60_01);
                    Message_StartTextbox(play, 0x45D, &this->actor);
                    this->curTextId = 0x45D; // exceeded 5000
                } else if (this->previousBankValue < (s16)HS_GET_BANK_RUPEES()) {
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                    Message_StartTextbox(play, 0x45E, &this->actor);
                    this->curTextId = 0x45E; // can't take more deposits
                } else {
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                    Message_StartTextbox(play, 0x460, &this->actor);
                    this->curTextId = 0x460; // come again
                }
            } else {
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                Message_StartTextbox(play, 0x460, &this->actor);
                this->curTextId = 0x460; // come again
            }
            break;

        case 0x45B: // given 200 reward
        case 0x45C: // given 1000 reward
        case 0x45D: // given 5000 reward
            this->isStampChecked = false;
            Message_CloseTextbox(play);
            EnGinkoMan_SetupBankAward(this);
            EnGinkoMan_BankAward(this, play);
            break;

        case 0x461: // whats your name (removed)
        case 0x462: // [name], is it? (removed)
        case 0x463: // gonna stamp (removed)
        case 0x464: // not gonna hurt (removed)
        case 0x465: // ur stamped (kept just in case stamp gets called unexpectedly)
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            play->msgCtx.rupeesTotal = HS_GET_BANK_RUPEES();
            Message_StartTextbox(play, 0x45A, &this->actor);
            this->curTextId = 0x45A; // show rupee total
            break;

        case 0x466: // greeting (removed)
        case 0x467: // greeting day 3 (removed)
            Message_StartTextbox(play, 0x468, &this->actor);
            this->curTextId = 0x468;
            break;

        case 0x469: // checking stamp (removed)
            EnGinkoMan_SetupStamp(this); // stamp player
            break;

        case 0x46A: // ah, yes...[name] (removed)
        case 0x46C: // ah, yes...[name] (night 3) (removed)
        case 0x47E: // deposits total [rupees]
            if (this->choiceDepositWithdrawl == GINKOMAN_CHOICE_DEPOSIT) {
                if (HS_GET_BANK_RUPEES() >= 5000) {
                    Message_StartTextbox(play, 0x45F, &this->actor);
                    this->curTextId = 0x45F; // excuuuse me (no more deposits)
                } else if (gSaveContext.save.saveInfo.playerData.rupees == 0) {
                    Message_StartTextbox(play, 0x458, &this->actor);
                    this->curTextId = 0x458; // you're broke
                } else {
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                    Message_StartTextbox(play, 0x450, &this->actor);
                    this->curTextId = 0x450; // deposit how much [prompt]
                }
            } else { // GINKOMAN_CHOICE_WITHDRAWL
                Message_StartTextbox(play, 0x46E, &this->actor);
                this->curTextId = 0x46E; // withdraw how much
            }

            this->choiceDepositWithdrawl = GINKOMAN_CHOICE_RESET;
            break;

        case 0x46B: // so... (removed)
            Message_StartTextbox(play, 0x46E, &this->actor);
            this->curTextId = 0x46E; // withdraw how much
            break;

        case 0x46D: // ignore the rumors (night 3) (removed)
            Message_StartTextbox(play, 0x46E, &this->actor);
            this->curTextId = 0x46E; // withdraw how much
            break;

        case 0x470: // is that so? come again
            if (Message_ShouldAdvance(play)) {
                Message_CloseTextbox(play);
                this->isStampChecked = false;
                EnGinkoMan_SetupIdle(this); // change to waiting for approach
            }
            break;

        case 0x476: // withdraw exceeds total
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
            FALLTHROUGH;
        case 0x475: // withdraw exceeds wallet space
        case 0x47C: // cancel withdrawl
        case 0x47D: // withdrawing nothing (duplicate?)
            Message_StartTextbox(play, 0x468, &this->actor);
            this->curTextId = 0x468; // deposit, withdraw or exit
            break;

        case 0x472: // tiny withdraw
        case 0x473: // use it wisely...
        case 0x474: // large withdraw
            if (HS_GET_BANK_RUPEES() == 0) {
                Message_StartTextbox(play, 0x478, &this->actor);
                this->curTextId = 0x478; // emptied account
            } else {
                play->msgCtx.rupeesTotal = HS_GET_BANK_RUPEES();
                Message_StartTextbox(play, 0x45A, &this->actor);
                this->curTextId = 0x45A; // show rupee total
            }
            break;

        case 0x477: // state withdrawl charge
            Message_StartTextbox(play, 0x471, &this->actor);
            this->curTextId = 0x471; // confirm withdrawl
            this->serviceFee = play->msgCtx.unk1206C;
            break;

        case 0x479: // gonna deposit?
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
            Message_StartTextbox(play, 0x450, &this->actor);
            this->curTextId = 0x450; // deposit how much [prompt]
            break;

        default:
            break;
    }
}

RECOMP_PATCH void EnGinkoMan_WaitForDialogueInput(EnGinkoMan* this, PlayState* play) {
    if (!Message_ShouldAdvance(play)) {
        return;
    }

    switch (this->curTextId) {
        case 0x44E: // deposit or exit
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_YES) {
                if (HS_GET_BANK_RUPEES() >= 5000) {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Message_StartTextbox(play, 0x45F, &this->actor);
                    this->curTextId = 0x45F; // bank full, cannot accept more
                } else {
                    if (gSaveContext.save.saveInfo.playerData.rupees > 0) {
                        Audio_PlaySfx_MessageDecide();
                        Message_StartTextbox(play, 0x450, &this->actor);
                        this->curTextId = 0x450; // deposit how much
                    } else {
                        Audio_PlaySfx(NA_SE_SY_ERROR);
                        Message_StartTextbox(play, 0x458, &this->actor);
                        this->curTextId = 0x458; // you're broke
                    }
                }
            } else { // GINKOMAN_CHOICE_NO
                Audio_PlaySfx_MessageCancel();
                Message_StartTextbox(play, 0x460, &this->actor);
                this->curTextId = 0x460; // come again
            }
            break;

        case 0x452: // confirm deposit
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_YES) {
                if (gSaveContext.save.saveInfo.playerData.rupees < play->msgCtx.rupeesSelected) {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                    Message_StartTextbox(play, 0x459, &this->actor);
                    this->curTextId = 0x459; // not enough in wallet
                } else {
                    Audio_PlaySfx_MessageDecide();
                    if (HS_GET_BANK_RUPEES() == 0) {
                        this->isNewAccount = true;
                    }

                    Rupees_ChangeBy(-play->msgCtx.rupeesSelected);
                    this->previousBankValue = HS_GET_BANK_RUPEES();
                    HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() + play->msgCtx.rupeesSelected);
                    play->msgCtx.rupeesTotal = HS_GET_BANK_RUPEES();
                    Message_StartTextbox(play, 0x45A, &this->actor);
                    this->curTextId = 0x45A; // show rupee total
                }
            } else { // GINKOMAN_CHOICE_NO
                Audio_PlaySfx_MessageCancel();
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                if (HS_GET_BANK_RUPEES() == 0) {
                    Message_StartTextbox(play, 0x44E, &this->actor);
                    this->curTextId = 0x44E; // deposit or exit
                } else {
                    Message_StartTextbox(play, 0x468, &this->actor);
                    this->curTextId = 0x468; // deposit, withdraw or exit
                }
            }
            break;

        case 0x468: // deposit, withdraw or exit
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_CANCEL) {
                Audio_PlaySfx_MessageCancel();
                Message_StartTextbox(play, 0x47C, &this->actor);
                this->curTextId = 0x47C; // cancel withdrawl
            } else {
                Audio_PlaySfx_MessageDecide();
                this->choiceDepositWithdrawl = play->msgCtx.choiceIndex;
                this->isStampChecked = true;
                Message_StartTextbox(play, 0x47E, &this->actor);
                this->curTextId = 0x47E; // deposits total [rupees]
            }
            break;

        case 0x471: // confirm withdrawl
            if (play->msgCtx.choiceIndex == GINKOMAN_CHOICE_YES) {
                if ((s32)HS_GET_BANK_RUPEES() < (play->msgCtx.rupeesSelected + this->serviceFee)) {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_LEGSMACKING);
                    Message_StartTextbox(play, 0x476, &this->actor);
                    this->curTextId = 0x476; // not enough to withdraw
                } else if (CUR_CAPACITY(UPG_WALLET) <
                           (play->msgCtx.rupeesSelected + gSaveContext.save.saveInfo.playerData.rupees)) {
                    // check if wallet is big enough
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                    Message_StartTextbox(play, 0x475, &this->actor);
                    this->curTextId = 0x475; // not enough wallet space
                } else {
                    Audio_PlaySfx_MessageDecide();
                    this->previousBankValue = HS_GET_BANK_RUPEES();
                    HS_SET_BANK_RUPEES(HS_GET_BANK_RUPEES() - play->msgCtx.rupeesSelected - this->serviceFee);
                    Rupees_ChangeBy(play->msgCtx.rupeesSelected);
                    if (HS_GET_BANK_RUPEES() == 0) {
                        Message_StartTextbox(play, 0x478, &this->actor);
                        this->curTextId = 0x478; // emptied account
                    } else {
                        play->msgCtx.rupeesTotal = HS_GET_BANK_RUPEES();
                        Message_StartTextbox(play, 0x45A, &this->actor);
                        this->curTextId = 0x45A; // show rupee total
                    }
                }
            } else {
                Audio_PlaySfx_MessageCancel();
                Message_StartTextbox(play, 0x47C, &this->actor);
                this->curTextId = 0x47C; // cancel withdrawl
            }
            break;

        default:
            break;
    }
}

RECOMP_PATCH void EnGinkoMan_WaitForRupeeCount(EnGinkoMan* this, PlayState* play) {
    if (Message_ShouldAdvance(play)) {
        switch (this->curTextId) {
            case 0x450: // deposit how much
                if (play->msgCtx.rupeesSelected == 0) {
                    if (HS_GET_BANK_RUPEES() == 0) {
                        Message_StartTextbox(play, 0x44E, &this->actor);
                        this->curTextId = 0x44E; // deposit or exit
                    } else {
                        Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                        Message_StartTextbox(play, 0x468, &this->actor);
                        this->curTextId = 0x468; // deposit, withdraw or exit
                    }
                } else {
                    Message_StartTextbox(play, 0x452, &this->actor);
                    this->curTextId = 0x452; // confirm deposit
                }
                break;

            case 0x46E: // withdraw how much
                if (play->msgCtx.rupeesSelected == 0) {
                    Message_StartTextbox(play, 0x47C, &this->actor);
                    this->curTextId = 0x47C; // cancel withdrawl
                } else if (gSaveContext.save.isNight == true) {
                    Message_StartTextbox(play, 0x477, &this->actor);
                    this->curTextId = 0x477; // state service charge // leave in for randomised amount?
                } else {
                    Message_StartTextbox(play, 0x471, &this->actor);
                    this->curTextId = 0x471; // confirm withdrawl
                    this->serviceFee = 0;
                }
                break;

            default:
                break;
        }
    }
}

// Should be unreachable, but just in case:
RECOMP_PATCH void EnGinkoMan_Stamp(EnGinkoMan* this, PlayState* play) {
    if ((this->curTextId == 0x464) && Animation_OnFrame(&this->skelAnime, 10.0f)) {
        Actor_PlaySfx(&this->actor, NA_SE_EV_HANKO); // "stamp"
    }

    if (Animation_OnFrame(&this->skelAnime, this->skelAnime.endFrame)) {
        switch (this->curTextId) {
            case 0x464: // not gonna hurt (removed)
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                Message_StartTextbox(play, 0x465, &this->actor);
                this->curTextId = 0x465; // stamped
                break;

            case 0x469: // checking stamp (removed)
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, GINKO_ANIM_SITTING);
                play->msgCtx.rupeesTotal = HS_GET_BANK_RUPEES();
                if ((CURRENT_DAY == 3) && (gSaveContext.save.isNight == true)) {
                    Message_StartTextbox(play, 0x46C, &this->actor);
                    this->curTextId = 0x46C; // ah, yes...[name] (night 3)
                } else {
                    Message_StartTextbox(play, 0x46A, &this->actor);
                    this->curTextId = 0x46A; // ah, yes..[name]
                }
                break;

            default:
                break;
        }

        EnGinkoMan_SetupDialogue(this);
    }
}
