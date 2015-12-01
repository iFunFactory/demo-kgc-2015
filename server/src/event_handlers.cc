// PLEASE ADD YOUR EVENT HANDLER DECLARATIONS HERE.

#include "event_handlers.h"

#include <map>

#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>

#include <funapi.h>
#include <glog/logging.h>

#include "unitychan_loggers.h"
#include "unitychan_messages.pb.h"


namespace unitychan {

namespace {
// 유저 상태
struct State {
  const int uid;   // 유저 아이디
  float av, ah;
  float px, pz;    // 현재 위치
  float ry, rw;
  bool jump;       // 점프 상태

  Ptr<Session> session;
  fun::WallClock::Value last_update;

  void Write(Spawn *spawn) const {
    spawn->set_user_id(uid);
    spawn->set_a_v(av);
    spawn->set_a_h(ah);
    spawn->set_p_x(px);
    spawn->set_p_z(pz);
    spawn->set_r_y(ry);
    spawn->set_r_w(rw);
  }

  void Write(UserState *state) const {
    state->set_user_id(uid);
    state->set_a_v(av);
    state->set_a_h(ah);
    state->set_p_x(px);
    state->set_p_z(pz);
    state->set_r_y(ry);
    state->set_r_w(rw);
  }
};


static boost::mutex the_user_lock;
static std::map<int, State> the_users;


void SendSpawn(const Ptr<Session> &session, const State &state) {
  Ptr<FunMessage> msg {new FunMessage()};
  auto *_msg = msg->MutableExtension(sc_spawn_self);
  state.Write(_msg->mutable_me());

  auto *others = _msg->mutable_others();
  for (auto &user: the_users) {
    if (state.uid != user.second.uid)
      user.second.Write(others->Add());
  }

  session->SendMessage("sc_spawn_self", msg);
}


void SendSpawnOther(const State &state) {
  Ptr<FunMessage> msg {new FunMessage()};
  state.Write(msg->MutableExtension(sc_spawn_other));

  for (auto &user: the_users) {
    if (state.uid != user.second.uid)
      user.second.session->SendMessage("sc_spawn_other", msg);
  }
}


void OnUserSpawned(const Ptr<Session> &session) {
  // 생성 위치는 (x, z) [-10 .. 10] 안에서 랜덤
  float px = static_cast<float>(fun::RandomGenerator::GenerateNumber(-10, 10));
  float pz = static_cast<float>(fun::RandomGenerator::GenerateNumber(-10, 10));

  const auto now = fun::WallClock::Now();
  boost::mutex::scoped_lock lock(the_user_lock);

  // uid는 아무도 없으면 1, 아니면 최대 값 + 1
  int uid = the_users.empty() ? 1 : the_users.rbegin()->first + 1;
  session->AddToContext("uid", uid);
  auto inserted = the_users.emplace(std::make_pair(
        uid, State{uid, 0.0f, 0.0f, px, pz, 0.0f, 0.0f, false, session, now}));
  BOOST_ASSERT(inserted.second);

  SendSpawn(session, inserted.first->second);
  SendSpawnOther(inserted.first->second);
}


void SendUserKilled(int32_t uid) {
  Ptr<FunMessage> msg {new FunMessage()};
  msg->MutableExtension(sc_killed)->set_user_id(uid);
  for (auto &user: the_users) {
    user.second.session->SendMessage("sc_killed", msg);
  }
}


void OnUserKilled(const Ptr<Session> &session) {
  int64_t uid;
  if (not session->GetFromContext("uid", &uid))
    return;  // uid가 없다

  {
    boost::mutex::scoped_lock lock(the_user_lock);
    auto user_it = the_users.find(uid);
    if (user_it == the_users.end())
      return;  // 해당 유저가 없다
    the_users.erase(user_it);
  }

  SendUserKilled(uid);
}


void OnInputUpdate(const Ptr<Session> &session, const Ptr<FunMessage> &msg) {
  if (not msg->HasExtension(cs_input))
    return;

  const auto &_msg = msg->GetExtension(cs_input);
  int64_t uid;
  if (not session->GetFromContext("uid", &uid))
    return;

  boost::mutex::scoped_lock lock(the_user_lock);
  auto user_it = the_users.find(uid);
  if (user_it == the_users.end())
    return;

  State &user = user_it->second;
  user.av = _msg.a_v();
  user.ah = _msg.a_h();
  user.px = _msg.p_x();
  user.pz = _msg.p_z();
  user.ry = _msg.r_y();
  user.rw = _msg.r_w();
  const bool jumped = (_msg.has_jump() && _msg.jump());

  Ptr<FunMessage> res {new FunMessage()};
  auto *users = res->MutableExtension(sc_update)->mutable_users();
  for (const auto &u: the_users) {
    auto *entry = users->Add();
    u.second.Write(entry);
    if (jumped && u.second.uid == uid) {
      entry->set_jump(jumped);
    }
  }

  for (const auto &u: the_users) {
    u.second.session->SendMessage("sc_update", res);
  }
}
////////////////////////////////////////////////////////////////////////////////
// Session open/close handlers
////////////////////////////////////////////////////////////////////////////////

void OnSessionOpened(const Ptr<Session> &session) {
  logger::SessionOpened(to_string(session->id()), WallClock::Now());

  OnUserSpawned(session);
}


void OnSessionClosed(const Ptr<Session> &session, SessionCloseReason reason) {
  logger::SessionClosed(to_string(session->id()), WallClock::Now());

  if (reason == kClosedForServerDid) {
    // Server has called session->Close().
  } else if (reason == kClosedForIdle) {
    // The session has been idle for long time.
  } else if (reason == kClosedForUnknownSessionId) {
    // The session was invalid.
  }

  OnUserKilled(session);
}


void OnTcpClosed(const Ptr<Session> &session) {
  session->Close();  // TCP 연결이 끊기면 session을 닫아버린다
}


////////////////////////////////////////////////////////////////////////////////
// Client message handlers.
//
// (Just for your reference. Please replace with your own.)
////////////////////////////////////////////////////////////////////////////////

void OnAccountLogin(const Ptr<Session> &session, const Json &message) {
  // Thank to a JSON schema we specified registering a handler,
  // we are guaranteed that the passsed "message" is in the following form.
  //
  //   {"facebook_uid":"xxx", "facebook_access_token":"xxx"}
  //
  // So no more validation is necessary.
  string fb_uid = message["facebook_uid"].GetString();
  string fb_access_token = message["facebook_access_token"].GetString();

  // Below shows how to initiate Facebook authentication.
  AuthenticationKey fb_auth_key = MakeFacebookAuthenticationKey(fb_access_token);

  AccountAuthenticationRequest request("Facebook", fb_uid, fb_auth_key);
  AccountAuthenticationResponse response;
  if (not AuthenticateSync(request, &response)) {
    // system error
    LOG(ERROR) << "authentication system error";
    return;
  }

  if (response.success) {
    // login success
    LOG(INFO) << "login success";

    // You can have the Engine manage logged-in accounts through "AccountManager".
    AccountManager::CheckAndSetLoggedIn(fb_uid, session);

    // We also leave a player activity log.
    // This log can be used when you need to do customer services.
    // To customize an activity log, please refer to the reference manual.
    logger::PlayerLoggedIn(to_string(session->id()), fb_uid, WallClock::Now());

  } else {
    // login failure
    LOG(INFO) << "login failure";
  }
}

}  // unnamed namespace


////////////////////////////////////////////////////////////////////////////////
// Extend the function below with your handlers.
////////////////////////////////////////////////////////////////////////////////

void RegisterEventHandlers() {
  /*
   * Registers handlers for session close/open events.
   */
  {
    HandlerRegistry::Install2(OnSessionOpened, OnSessionClosed);
    HandlerRegistry::RegisterTcpTransportDetachedHandler(OnTcpClosed);
  }


  /*
   * Registers handlers for messages from the client.
   *
   * Handlers below are just for you reference.
   * Feel free to delete them and replace with your own.
   */
  {
    // 1. Registering a JSON message named "login" with its JSON schema.
    //    With json schema, Engine validates input messages in JSON.
    //    before entering a handler.
    //    You can specify a JSON schema like below, or you can also use
    //    auxiliary files in src/json_protocols directory.
    JsonSchema login_msg(JsonSchema::kObject,
        JsonSchema("facebook_uid", JsonSchema::kString, true),
        JsonSchema("facebook_access_token", JsonSchema::kString, true));

    HandlerRegistry::Register("login", OnAccountLogin, login_msg);
    /////////////////////////////////////////////
    // PLACE YOUR CLIENT MESSAGE HANDLER HERE. //
    /////////////////////////////////////////////
    HandlerRegistry::Register2("cs_input", OnInputUpdate);
  }
}

}  // namespace unitychan
