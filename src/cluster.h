#pragma once

#include "common.h"

namespace nl {

class ClusterWrap : public Napi::ObjectWrap<ClusterWrap> {
 public:
  static Napi::FunctionReference constructor;
  static void Init(Napi::Env env, Napi::Object exports);
  static Napi::Object NewInstance(Napi::Env env, std::shared_ptr<ClusterState> state);

  explicit ClusterWrap(const Napi::CallbackInfo& info);
  std::shared_ptr<ClusterState> state() const { return state_; }

 private:
  Napi::Value ConfigReadFile(const Napi::CallbackInfo& info);
  Napi::Value ConfigParseEnv(const Napi::CallbackInfo& info);
  Napi::Value ConfigGet(const Napi::CallbackInfo& info);
  Napi::Value ConfigSet(const Napi::CallbackInfo& info);
  Napi::Value Connect(const Napi::CallbackInfo& info);
  Napi::Value Close(const Napi::CallbackInfo& info);
  Napi::Value IsClosed(const Napi::CallbackInfo& info);
  Napi::Value Fsid(const Napi::CallbackInfo& info);
  Napi::Value InstanceId(const Napi::CallbackInfo& info);
  Napi::Value Stats(const Napi::CallbackInfo& info);
  Napi::Value Pools(const Napi::CallbackInfo& info);
  Napi::Value PoolLookup(const Napi::CallbackInfo& info);
  Napi::Value PoolReverseLookup(const Napi::CallbackInfo& info);
  Napi::Value PoolCreate(const Napi::CallbackInfo& info);
  Napi::Value PoolDelete(const Napi::CallbackInfo& info);
  Napi::Value OpenIoContext(const Napi::CallbackInfo& info);
  Napi::Value Command(const Napi::CallbackInfo& info);
  Napi::Value PingMonitor(const Napi::CallbackInfo& info);
  Napi::Value WaitForLatestOsdMap(const Napi::CallbackInfo& info);
  Napi::Value BlocklistAdd(const Napi::CallbackInfo& info);
  Napi::Value ServiceRegister(const Napi::CallbackInfo& info);
  Napi::Value ServiceUpdateStatus(const Napi::CallbackInfo& info);
  Napi::Value MonitorLog(const Napi::CallbackInfo& info);

  std::shared_ptr<ClusterState> state_;
};

}  // namespace nl
