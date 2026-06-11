#include "Player.h"
#include "Projectile.h"
#include "AudioManager.h"
#include "AssetManager.h"
//#include <SDL2/SDL.h>
#include <SDL.h>
#include <algorithm>
#include <vector>

namespace Mario {

Player::Player() : Entity(EntityType::Player)
{
    bounds_ = {100, 400, 28, 32};
}

void Player::update(float dt)
{
    if (state_ == PlayerState::Die) {
        dieTime_ -= dt;
        vel_.y   += GRAVITY * dt * 0.5f;
        bounds_.y += vel_.y * dt;
        if (dieTime_ <= 0) kill();
        return;
    }

    if (invTime_ > 0) {
        invTime_ -= dt;
        if (invTime_ < 0) {
            invTime_ = 0;
            state_   = PlayerState::Idle;
        }
    }

    if (shootCd_ > 0) shootCd_ -= dt;

    applyInput(dt);
    clampVelocity();
    updateAnimation(dt);
}
//Accepts three input sources for every action: arrow keys, WASD, and the touch virtual button. Any one of them activates the movement.
void Player::applyInput(float dt)
{
    if (!input_) return;

    bool left  = input_->isDown(SDL_SCANCODE_LEFT)  ||
                 input_->isDown(SDL_SCANCODE_A)      ||
                 input_->touchLeft();
    bool right = input_->isDown(SDL_SCANCODE_RIGHT) ||
                 input_->isDown(SDL_SCANCODE_D)      ||
                 input_->touchRight();
    bool jump  = input_->isPressed(SDL_SCANCODE_SPACE)||
                 input_->isPressed(SDL_SCANCODE_UP)   ||
                 input_->isPressed(SDL_SCANCODE_W)     ||
                 input_->touchJump();
    bool jumpHeld = input_->isDown(SDL_SCANCODE_SPACE)||
                    input_->touchJump();
    running_   = input_->isDown(SDL_SCANCODE_LSHIFT) ||
                 input_->isDown(SDL_SCANCODE_Z)       ||
                 input_->touchRun();
//Instead of setting vel_.x directly to spd, we accelerate toward it. spd * 8 * dt means the player reaches full speed in about 0.125 seconds — feels snappy without being instant. std::min caps at the top speed.
    float spd = running_ ? PLAYER_RUN : PLAYER_SPEED;

    /* horizontal movement */
    if (right) {
        vel_.x  = std::min(vel_.x + spd * 8 * dt, spd);
        facingRight_ = true;
        state_   = running_ ? PlayerState::Run : PlayerState::Walk;
    } else if (left) {
        vel_.x  = std::max(vel_.x - spd * 8 * dt, -spd);
        facingRight_ = false;
        state_   = running_ ? PlayerState::Run : PlayerState::Walk;
    } else {//When no direction is held, velocity is multiplied by 0.82 each frame — exponential friction that feels natural. The threshold 5 prevents the player from sliding forever at sub-pixel speeds.
        /* friction */
        vel_.x  *= 0.82f;
        if (std::abs(vel_.x) < 5) vel_.x = 0;
        if (onGround_) state_ = PlayerState::Idle;
    }

    /* jump */
    if (jump && onGround_) {//Jump is only allowed onGround_ — prevents double-jumping. Immediately sets onGround_ to false so a second jump press in the same frame doesn't trigger again.
        vel_.y  = JUMP_FORCE;
        onGround_ = false;
        AudioManager::instance().playSound("jump");
        state_  = PlayerState::Jump;
    }
    /* hold jump for higher jump */
    if (!jumpHeld && vel_.y < -300.0f)//Variable-height jump: releasing the jump button while still moving upward cuts the vertical velocity. This gives the player control over jump height — tap for a small hop, hold for a full jump.
        vel_.y = -300.0f;

    /* air state */
    if (!onGround_)
        state_ = (vel_.y < 0) ? PlayerState::Jump : PlayerState::Fall;

    /* fire */
    if (power_ == PowerLevel::Fire && shootCd_ <= 0) {
        if (input_->isPressed(SDL_SCANCODE_LCTRL) ||
            input_->isPressed(SDL_SCANCODE_X)) {
            shootCd_ = 0.25f;
            AudioManager::instance().playSound("fireball");
        }
    }
}

void Player::clampVelocity()
{
    vel_.x = std::clamp(vel_.x, -PLAYER_RUN, PLAYER_RUN);
    vel_.y = std::clamp(vel_.y, JUMP_FORCE,  800.0f);
}

void Player::updateAnimation(float dt)
{
    frameTime_ += dt;
    float dur = (state_ == PlayerState::Run) ? 0.07f : 0.1f;
    if (frameTime_ >= dur) {
        frameTime_ = 0;
        frame_ = (frame_ + 1) % 4;
    }
}

void Player::render(SDL_Renderer* r, const Vec2& cam)
{
    /* invincibility blink */
    if (invTime_ > 0 && (int)(invTime_ * 10) % 2 == 0) return;

    SDL_Rect dst = {
        (int)(bounds_.x - cam.x),
        (int)(bounds_.y - cam.y),
        (int)bounds_.w, (int)bounds_.h
    };

    /* draw player as coloured rectangle with hat (placeholder) */
    /* Red body */
    SDL_SetRenderDrawColor(r, 220, 60, 40, 255);
    SDL_RenderFillRect(r, &dst);

    /* Blue overalls */
    SDL_Rect overall = {dst.x, dst.y + dst.h/2,
                         dst.w, dst.h/2};
    SDL_SetRenderDrawColor(r, 40, 80, 200, 255);
    SDL_RenderFillRect(r, &overall);

    /* Hat */
    SDL_Rect hat = {dst.x, dst.y - 8, dst.w, 10};
    SDL_SetRenderDrawColor(r, 220, 60, 40, 255);
    SDL_RenderFillRect(r, &hat);

    /* Eyes */
    SDL_Rect eye = {
        dst.x + (facingRight_ ? dst.w-8 : 4),
        dst.y + 4, 5, 5
    };
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_RenderFillRect(r, &eye);

    /* power level indicator */
    if (power_ == PowerLevel::Fire) {
        SDL_Rect fire = {dst.x, dst.y-2, 6, 6};
        SDL_SetRenderDrawColor(r, 255, 200, 0, 255);
        SDL_RenderFillRect(r, &fire);
    }
}

void Player::takeDamage()//When big/fire Mario takes damage, he shrinks instead of dying. invTime_ = 2.0f grants 2 seconds of invincibility (blinking) so the player can recover safely.
{
    if (invTime_ > 0) return;

    if (power_ != PowerLevel::Small) {
        power_   = PowerLevel::Small;
        bounds_.h = 32;
        invTime_  = 2.0f;
        AudioManager::instance().playSound("powerdown");
    } else {//Small Mario dying: the die state launches him upward (classic bounce-off-screen animation), stops horizontal movement, and decrements lives. The dieTime_ timer controls how long until the game state changes.
        state_   = PlayerState::Die;
        vel_.y   = -500.0f;
        vel_.x   = 0;
        dieTime_ = 3.0f;
        lives_--;
        AudioManager::instance().playSound("die");
    }
}

void Player::onLanding() { onGround_ = true; }

void Player::collectCoin()//Every 100 coins grants an extra life — classic Mario mechanic. The if check and decrement (-= 100 instead of = 0) correctly handles collecting multiple coins simultaneously without losing any.
{
    coins_++;
    score_ += 200;
    AudioManager::instance().playSound("coin");
    if (coins_ >= 100) {
        coins_ -= 100;
        lives_++;
        AudioManager::instance().playSound("1up");
    }
}

void Player::grow()
{
    if (power_ == PowerLevel::Small) {
        power_    = PowerLevel::Big;
        bounds_.h = 60;
        AudioManager::instance().playSound("powerup");
    }
}

void Player::gainFirePower()
{
    power_ = PowerLevel::Fire;
    AudioManager::instance().playSound("powerup");
}

bool Player::canShoot() const
{
    return power_ == PowerLevel::Fire && shootCd_ <= 0;
}

void Player::onCollideWith(Entity* other) {}

} /* namespace Mario */
