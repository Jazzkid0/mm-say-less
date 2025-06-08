#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"
#include "attributes.h"

#include "overlays/actors/ovl_En_Bal/z_en_bal.h"
#include "overlays/actors/ovl_En_Clear_Tag/z_en_clear_tag.h"

typedef enum {
    /* 0 */ TINGLE_EYETEX_OPEN,
    /* 1 */ TINGLE_EYETEX_CLOSED
} TingleEyeTexture;

typedef enum {
    /* -1 */ TINGLE_ANIM_NONE = -1,
    /*  0 */ TINGLE_ANIM_FLOAT_IDLE,
    /*  1 */ TINGLE_ANIM_FALL_LOOP,
    /*  2 */ TINGLE_ANIM_FALL_ONCE,
    /*  3 */ TINGLE_ANIM_LAND,
    /*  4 */ TINGLE_ANIM_TWIST,
    /*  5 */ TINGLE_ANIM_TALK,
    /*  6 */ TINGLE_ANIM_MAGIC,
    /*  7 */ TINGLE_ANIM_HAPPY_DANCE_LOOP,
    /*  8 */ TINGLE_ANIM_HAPPY_DANCE_ONCE,
    /*  9 */ TINGLE_ANIM_MAGIC_REVERSE,
    /* 10 */ TINGLE_ANIM_IDLE,
    /* 11 */ TINGLE_ANIM_SPIN,
    /* 12 */ TINGLE_ANIM_HIDE_FACE,
    /* 13 */ TINGLE_ANIM_CONFETTI,
    /* 14 */ TINGLE_ANIM_MAX
} TingleAnimation;

typedef enum {
    /* 0 */ TINGLE_MAPCHOICE_PROXIMAL,
    /* 1 */ TINGLE_MAPCHOICE_DISTAL,
    /* 2 */ TINGLE_MAPCHOICE_CANCEL
} TingleBuyMapChoice;

typedef enum {
    /* 0 */ TINGLE_WATCH_TARGET_NONE,
    /* 1 */ TINGLE_WATCH_TARGET_PLAYER,
    /* 2 */ TINGLE_WATCH_TARGET_FAIRY
} TingleWatchTarget;

typedef enum {
    /* 0 */ TINGLE_BALLOON_ACTION_NONE,
    /* 1 */ TINGLE_BALLOON_ACTION_POP,
    /* 2 */ TINGLE_BALLOON_ACTION_FALL,
    /* 4 */ TINGLE_BALLOON_ACTION_INFLATE = 4,
    /* 5 */ TINGLE_BALLOON_ACTION_RISE
} TingleBalloonAction;

typedef enum {
    /* 0 */ TINGLE_IDLESTAGE_ACTIVITY,
    /* 2 */ TINGLE_IDLESTAGE_PREP_WAIT = 2,
    /* 3 */ TINGLE_IDLESTAGE_WAIT
} TingleIdleAnimStage;

extern void EnBal_SetMainColliderToHead(EnBal* this);
extern s32 EnBal_ValidatePictograph(PlayState* play, Actor* thisx);
extern void EnBal_SetupFloatIdle(EnBal* this);
extern void EnBal_FloatIdle(EnBal* this, PlayState* play);
extern void EnBal_PopBalloon(EnBal* this, PlayState* play);
extern void EnBal_SetupFall(EnBal* this);
extern void EnBal_Fall(EnBal* this, PlayState* play);
extern void EnBal_InflateBalloon(EnBal* this, PlayState* play);
extern void EnBal_SetupFloatUp(EnBal* this);
extern void EnBal_FloatUp(EnBal* this, PlayState* play);
extern void EnBal_SetupGroundIdle(EnBal* this);
extern void EnBal_GroundIdle(EnBal* this, PlayState* play);
extern void EnBal_SetupTalk(EnBal* this);
extern void EnBal_Talk(EnBal* this, PlayState* play);
extern PlayerTransformation EnBal_GetRecognizedPlayerForm(void);
extern void EnBal_ThrowMagicSparkles(EnBal* this, PlayState* play);
extern void EnBal_EmitDustPuff(EnBal* this, PlayState* play);
extern void EnBal_TryPurchaseMap(EnBal* this, PlayState* play);
extern void EnBal_HandleConversation(EnBal* this, PlayState* play);
extern void EnBal_SetupOfferGetItem(EnBal* this);
extern void EnBal_OfferGetItem(EnBal* this, PlayState* play);
extern void EnBal_SetupThankYou(EnBal* this);
extern void EnBal_ThankYou(EnBal* this, PlayState* play);

extern void EnBal_SetupInflateBalloon(EnBal* this);
extern s32 EnBal_CheckIfMapUnlocked(EnBal* this, PlayState* play);
extern void EnBal_UnlockSelectedAreaMap(EnBal* this);

static s16 sBuyMapOptions[TINGLE_MAP_MAX][2] = {
    { TINGLE_MAP_CLOCK_TOWN, TINGLE_MAP_WOODFALL },    // TINGLE_MAP_CLOCK_TOWN
    { TINGLE_MAP_WOODFALL, TINGLE_MAP_SNOWHEAD },      // TINGLE_MAP_WOODFALL
    { TINGLE_MAP_SNOWHEAD, TINGLE_MAP_ROMANI_RANCH },  // TINGLE_MAP_SNOWHEAD
    { TINGLE_MAP_ROMANI_RANCH, TINGLE_MAP_GREAT_BAY }, // TINGLE_MAP_ROMANI_RANCH
    { TINGLE_MAP_GREAT_BAY, TINGLE_MAP_STONE_TOWER },  // TINGLE_MAP_GREAT_BAY
    { TINGLE_MAP_STONE_TOWER, TINGLE_MAP_CLOCK_TOWN }, // TINGLE_MAP_STONE_TOWER
};

static AnimationInfo sAnimationInfo[TINGLE_ANIM_MAX] = {
    { &gTingleFloatIdleAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f },          // TINGLE_ANIM_FLOAT_IDLE
    { &gTingleFallAnim, 1.5f, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f },               // TINGLE_ANIM_FALL_LOOP
    { &gTingleFallAnim, 1.5f, 0.0f, 0.0f, ANIMMODE_ONCE, -4.0f },               // TINGLE_ANIM_FALL_ONCE
    { &gTingleLandAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -4.0f },               // TINGLE_ANIM_LAND
    { &gTingleTwistAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f },              // TINGLE_ANIM_TWIST
    { &gTingleTalkAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f },               // TINGLE_ANIM_TALK
    { &gTingleThrowConfettiAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -8.0f },      // TINGLE_ANIM_MAGIC
    { &gTingleHappyDanceAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f },         // TINGLE_ANIM_HAPPY_DANCE_LOOP
    { &gTingleHappyDanceAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_ONCE, -8.0f },         // TINGLE_ANIM_HAPPY_DANCE_ONCE
    { &gTingleThrowConfettiAnim, 1.0f, 23.0f, 0.0f, ANIMMODE_ONCE, -8.0f },     // TINGLE_ANIM_MAGIC_REVERSE
    { &gTingleIdleAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -8.0f },               // TINGLE_ANIM_IDLE
    { &gTingleSpinAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -2.0f },               // TINGLE_ANIM_SPIN
    { &gTingleFloatHideFaceAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -2.0f },      // TINGLE_ANIM_HIDE_FACE
    { &gTingleFloatThrowConfettiAnim, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP, -2.0f }, // TINGLE_ANIM_CONFETTI
};


RECOMP_PATCH void EnBal_GroundIdle(EnBal* this, PlayState* play) {
    Player* player = GET_PLAYER(play);

    if (this->timer == 300) {
        if (Animation_OnFrame(&this->skelAnime, this->skelAnime.endFrame)) {
            EnBal_SetupInflateBalloon(this);
            return;
        }
    } else if (this->timer < 300) {
        this->timer++;
    }

    if (Actor_TalkOfferAccepted(&this->picto.actor, &play->state)) {
        this->forceEyesShut = false;
        this->eyeTexIndex = TINGLE_EYETEX_OPEN;
        this->watchTarget = TINGLE_WATCH_TARGET_PLAYER;
        Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
        Message_StartTextbox(play, 0x1D04, &this->picto.actor);
        this->textId = 0x1D04;
        EnBal_SetupTalk(this);
    } else {
        if ((this->picto.actor.xzDistToPlayer < 100.0f) && (this->actionFunc != EnBal_InflateBalloon)) {
            if (this->idleAnimStage != TINGLE_IDLESTAGE_WAIT) {
                this->watchTarget = TINGLE_WATCH_TARGET_PLAYER;
            } else {
                this->watchTarget = TINGLE_WATCH_TARGET_NONE;
            }

            if (!(player->stateFlags1 & PLAYER_STATE1_800000) && !(player->actor.bgCheckFlags & BGCHECKFLAG_WATER) &&
                ((this->timer < 300) || (this->timer == 301))) {
                Actor_OfferTalk(&this->picto.actor, play, 100.0f);
            }
        } else {
            this->watchTarget = TINGLE_WATCH_TARGET_NONE;
        }

        if (Animation_OnFrame(&this->skelAnime, this->skelAnime.endFrame)) {
            if (this->idleAnimStage == TINGLE_IDLESTAGE_PREP_WAIT) {
                this->forceEyesShut = false;
                this->eyeTexIndex = TINGLE_EYETEX_OPEN;
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_IDLE);
                this->idleAnimStage++;
            } else if (this->idleAnimStage == TINGLE_IDLESTAGE_WAIT) {
                if (Rand_Next() & 1) {
                    this->forceEyesShut = false;
                    this->eyeTexIndex = TINGLE_EYETEX_OPEN;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
                } else {
                    this->forceEyesShut = true;
                    Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_SPIN);
                }
                this->idleAnimStage = TINGLE_IDLESTAGE_ACTIVITY;
            } else {
                this->idleAnimStage++;
            }
        } else if ((this->idleAnimStage == TINGLE_IDLESTAGE_WAIT) && Animation_OnFrame(&this->skelAnime, 20.0f)) {
            this->forceEyesShut = true;
        }
    }
}

RECOMP_PATCH void EnBal_TryPurchaseMap(EnBal* this, PlayState* play) {
    Player* player = GET_PLAYER(play);
    s32 price;

    if (Message_ShouldAdvance(play)) {
        if (play->msgCtx.choiceIndex != TINGLE_MAPCHOICE_CANCEL) {
            // Get price depending on which map player wants to buy
            if (play->msgCtx.choiceIndex == TINGLE_MAPCHOICE_PROXIMAL) {
                price = play->msgCtx.unk1206C;
            } else {
                price = play->msgCtx.unk12070;
            }

            if (gSaveContext.save.saveInfo.playerData.rupees < price) {
                // Can't buy map because player doesn't have the money
                Audio_PlaySfx(NA_SE_SY_ERROR);
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
                Message_StartTextbox(play, 0x1D0A, &this->picto.actor);
                this->textId = 0x1D0A;
            } else if (EnBal_CheckIfMapUnlocked(this, play)) {
                // Can't buy map because player already has it
                Audio_PlaySfx(NA_SE_SY_ERROR);
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
                Message_StartTextbox(play, 0x1D09, &this->picto.actor);
                this->textId = 0x1D09;
            } else {
                // Proceed with map purchase
                Audio_PlaySfx_MessageDecide();
                Rupees_ChangeBy(-price);
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_MAGIC_REVERSE);
                this->forceEyesShut = true;
                Message_StartTextbox(play, 0x1D0B, &this->picto.actor);
                this->textId = 0x1D0B;
                EnBal_UnlockSelectedAreaMap(this);
                player->stateFlags1 |= PLAYER_STATE1_20;
                EnBal_SetupOfferGetItem(this);
            }
        } else {
            // Cancel
            Audio_PlaySfx_MessageCancel();
            Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
            Message_StartTextbox(play, 0x1D06, &this->picto.actor);
            this->textId = 0x1D06; // I see! call again
        }
    }
}

RECOMP_PATCH void EnBal_HandleConversation(EnBal* this, PlayState* play) {
    if (Message_ShouldAdvance(play)) {
        switch (this->textId) {
            case 0x1D00: // (removed)
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
                Message_StartTextbox(play, 0x1D01, &this->picto.actor);
                this->textId = 0x1D01;
                break;

            case 0x1D01: // (removed)
                this->watchTarget = TINGLE_WATCH_TARGET_PLAYER;
                Message_StartTextbox(play, 0x1D02, &this->picto.actor);
                this->textId = 0x1D02;
                break;

            case 0x1D02: // (removed)
                this->forceEyesShut = true;
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_HAPPY_DANCE_LOOP);
                Message_StartTextbox(play, 0x1D03, &this->picto.actor);
                this->textId = 0x1D03;
                break;

            case 0x1D03: // (removed)
            case 0x1D0D: // (removed)
                this->forceEyesShut = false;
                this->eyeTexIndex = TINGLE_EYETEX_OPEN;
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_TALK);
                Message_StartTextbox(play, 0x1D04, &this->picto.actor);
                this->textId = 0x1D04;
                break;

            case 0x1D04: // Will you buy a map?
                this->watchTarget = TINGLE_WATCH_TARGET_PLAYER;
                switch (this->locationMapId) {
                    case TINGLE_MAP_CLOCK_TOWN:
                        Message_StartTextbox(play, 0x1D11, &this->picto.actor);
                        this->textId = 0x1D11;
                        break;

                    case TINGLE_MAP_WOODFALL:
                        Message_StartTextbox(play, 0x1D12, &this->picto.actor);
                        this->textId = 0x1D12;
                        break;

                    case TINGLE_MAP_SNOWHEAD:
                        Message_StartTextbox(play, 0x1D13, &this->picto.actor);
                        this->textId = 0x1D13;
                        break;

                    case TINGLE_MAP_ROMANI_RANCH:
                        Message_StartTextbox(play, 0x1D14, &this->picto.actor);
                        this->textId = 0x1D14;
                        break;

                    case TINGLE_MAP_GREAT_BAY:
                        Message_StartTextbox(play, 0x1D15, &this->picto.actor);
                        this->textId = 0x1D15;
                        break;

                    case TINGLE_MAP_STONE_TOWER:
                        Message_StartTextbox(play, 0x1D16, &this->picto.actor);
                        this->textId = 0x1D16;
                        break;

                    default:
                        Message_StartTextbox(play, 0x1D11, &this->picto.actor);
                        this->textId = 0x1D11;
                        break;
                }
                break;

            case 0x1D05:
            case 0x1D0C:
                this->watchTarget = TINGLE_WATCH_TARGET_FAIRY;
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_HAPPY_DANCE_LOOP);
                Message_StartTextbox(play, 0x1D0D, &this->picto.actor);
                this->textId = 0x1D0D;
                break;

            case 0x1D06:
            case 0x1D17:
                this->watchTarget = TINGLE_WATCH_TARGET_NONE;
                Actor_ChangeAnimationByInfo(&this->skelAnime, sAnimationInfo, TINGLE_ANIM_MAGIC);
                this->forceEyesShut = true;
                Message_StartTextbox(play, 0x1D07, &this->picto.actor);
                this->textId = 0x1D07;
                break;

            case 0x1D07:
                Message_CloseTextbox(play);
                EnBal_SetupGroundIdle(this);
                break;

            case 0x1D08:
            case 0x1D09:
            case 0x1D0A:
                Message_CloseTextbox(play);
                EnBal_SetupGroundIdle(this);
                break;

            default:
                break;
        }
    }
}
