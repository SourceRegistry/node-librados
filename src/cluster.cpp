#include "cluster.h"

#include "io_context.h"
#include "events.h"

#include <array>
#include <cstdlib>

namespace nl {

Napi::FunctionReference ClusterWrap::constructor;

namespace {
struct CommandResult { std::vector<uint8_t> output; std::string status; };

std::vector<std::string> js_strings(const Napi::Value& value, const char* name) {
  if (!value.IsArray()) throw Napi::TypeError::New(value.Env(), std::string(name) + " must be an array");
  auto array = value.As<Napi::Array>();
  std::vector<std::string> result;
  result.reserve(array.Length());
  for (uint32_t i = 0; i < array.Length(); ++i) {
    if (!array.Get(i).IsString()) throw Napi::TypeError::New(value.Env(), std::string(name) + " entries must be strings");
    result.push_back(array.Get(i).As<Napi::String>().Utf8Value());
  }
  return result;
}

std::vector<uint8_t> js_buffer(const Napi::Value& value, const char* name) {
  if (value.IsUndefined() || value.IsNull()) return {};
  if (!value.IsBuffer()) throw Napi::TypeError::New(value.Env(), std::string(name) + " must be a Buffer");
  auto buffer = value.As<Napi::Buffer<uint8_t>>();
  return {buffer.Data(), buffer.Data() + buffer.Length()};
}
}

void ClusterWrap::Init(Napi::Env env, Napi::Object exports) {
  auto function = DefineClass(env, "NativeCluster", {
      InstanceMethod("configReadFile", &ClusterWrap::ConfigReadFile),
      InstanceMethod("configParseEnv", &ClusterWrap::ConfigParseEnv),
      InstanceMethod("configGet", &ClusterWrap::ConfigGet),
      InstanceMethod("configSet", &ClusterWrap::ConfigSet),
      InstanceMethod("connect", &ClusterWrap::Connect),
      InstanceMethod("close", &ClusterWrap::Close),
      InstanceAccessor("closed", &ClusterWrap::IsClosed, nullptr),
      InstanceMethod("fsid", &ClusterWrap::Fsid),
      InstanceAccessor("instanceId", &ClusterWrap::InstanceId, nullptr),
      InstanceMethod("stats", &ClusterWrap::Stats),
      InstanceMethod("pools", &ClusterWrap::Pools),
      InstanceMethod("poolLookup", &ClusterWrap::PoolLookup),
      InstanceMethod("poolReverseLookup", &ClusterWrap::PoolReverseLookup),
      InstanceMethod("poolCreate", &ClusterWrap::PoolCreate),
      InstanceMethod("poolDelete", &ClusterWrap::PoolDelete),
      InstanceMethod("openIoContext", &ClusterWrap::OpenIoContext),
      InstanceMethod("command", &ClusterWrap::Command),
      InstanceMethod("pingMonitor", &ClusterWrap::PingMonitor),
      InstanceMethod("waitForLatestOsdMap", &ClusterWrap::WaitForLatestOsdMap),
      InstanceMethod("blocklistAdd", &ClusterWrap::BlocklistAdd),
      InstanceMethod("serviceRegister", &ClusterWrap::ServiceRegister),
      InstanceMethod("serviceUpdateStatus", &ClusterWrap::ServiceUpdateStatus),
      InstanceMethod("monitorLog", &ClusterWrap::MonitorLog),
  });
  constructor = Napi::Persistent(function);
  constructor.SuppressDestruct();
  exports.Set("NativeCluster", function);
}

Napi::Object ClusterWrap::NewInstance(Napi::Env env, std::shared_ptr<ClusterState> state) {
  auto* holder = new std::shared_ptr<ClusterState>(std::move(state));
  return constructor.New({Napi::External<std::shared_ptr<ClusterState>>::New(env, holder)});
}

ClusterWrap::ClusterWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<ClusterWrap>(info) {
  if (info.Length() == 1 && info[0].IsExternal()) {
    auto* holder = info[0].As<Napi::External<std::shared_ptr<ClusterState>>>().Data();
    state_ = std::move(*holder);
    delete holder;
    return;
  }
  const std::string cluster_name = info.Length() > 0 && info[0].IsString()
      ? info[0].As<Napi::String>().Utf8Value() : "ceph";
  const std::string user_name = info.Length() > 1 && info[1].IsString()
      ? info[1].As<Napi::String>().Utf8Value() : "client.admin";
  rados_t handle = nullptr;
  int result = rados_create2(&handle, cluster_name.c_str(), user_name.c_str(), 0);
  if (result < 0) {
    throw_ceph(info.Env(), result, "rados_create2", cluster_name + "/" + user_name);
    return;
  }
  state_ = std::make_shared<ClusterState>(handle, [](rados_t value) { rados_shutdown(value); return 0; });
}

Napi::Value ClusterWrap::ConfigReadFile(const Napi::CallbackInfo& info) {
  std::optional<std::string> path;
  if (info.Length() && !info[0].IsNull() && !info[0].IsUndefined()) path = string_arg(info, 0, "path");
  int result = state_->use([&](rados_t h) { return rados_conf_read_file(h, path ? path->c_str() : nullptr); });
  if (result < 0) throw_ceph(info.Env(), result, "rados_conf_read_file", path.value_or("default search path"));
  return info.Env().Undefined();
}

Napi::Value ClusterWrap::ConfigParseEnv(const Napi::CallbackInfo& info) {
  std::optional<std::string> variable;
  if (info.Length() && !info[0].IsNull() && !info[0].IsUndefined()) variable = string_arg(info, 0, "variable");
  int result = state_->use([&](rados_t h) { return rados_conf_parse_env(h, variable ? variable->c_str() : nullptr); });
  if (result < 0) throw_ceph(info.Env(), result, "rados_conf_parse_env", variable.value_or("CEPH_ARGS"));
  return info.Env().Undefined();
}

Napi::Value ClusterWrap::ConfigGet(const Napi::CallbackInfo& info) {
  const auto key = string_arg(info, 0, "key");
  std::vector<char> buffer(256);
  int result;
  while ((result = state_->use([&](rados_t h) { return rados_conf_get(h, key.c_str(), buffer.data(), buffer.size()); })) == -ENAMETOOLONG)
    buffer.resize(buffer.size() * 2);
  if (result < 0) { throw_ceph(info.Env(), result, "rados_conf_get", key); return info.Env().Undefined(); }
  return Napi::String::New(info.Env(), buffer.data());
}

Napi::Value ClusterWrap::ConfigSet(const Napi::CallbackInfo& info) {
  const auto key = string_arg(info, 0, "key");
  const auto value = string_arg(info, 1, "value");
  int result = state_->use([&](rados_t h) { return rados_conf_set(h, key.c_str(), value.c_str()); });
  if (result < 0) throw_ceph(info.Env(), result, "rados_conf_set", key);
  return info.Env().Undefined();
}

Napi::Value ClusterWrap::Connect(const Napi::CallbackInfo& info) {
  auto state = state_;
  return async_void(info.Env(), "rados_connect", {}, [state] {
    return state->use([](rados_t h) { return rados_connect(h); });
  });
}

Napi::Value ClusterWrap::Close(const Napi::CallbackInfo& info) {
  auto state = state_;
  return async_void(info.Env(), "rados_shutdown", {}, [state] { return state->close_checked(); });
}

Napi::Value ClusterWrap::IsClosed(const Napi::CallbackInfo& info) {
  return Napi::Boolean::New(info.Env(), state_->closed());
}

Napi::Value ClusterWrap::Fsid(const Napi::CallbackInfo& info) {
  auto state = state_;
  return async_value<std::string>(info.Env(), "rados_cluster_fsid", {},
      [state](std::string& out) {
        std::array<char, 64> buffer{};
        int result = state->use([&](rados_t h) { return rados_cluster_fsid(h, buffer.data(), buffer.size()); });
        if (result >= 0) out = buffer.data();
        return result;
      }, [](Napi::Env env, std::string& value) { return Napi::String::New(env, value); });
}

Napi::Value ClusterWrap::InstanceId(const Napi::CallbackInfo& info) {
  uint64_t id = state_->use([](rados_t h) { return rados_get_instance_id(h); });
  return Napi::BigInt::New(info.Env(), id);
}

Napi::Value ClusterWrap::Stats(const Napi::CallbackInfo& info) {
  auto state = state_;
  return async_value<rados_cluster_stat_t>(info.Env(), "rados_cluster_stat", {},
      [state](rados_cluster_stat_t& out) { return state->use([&](rados_t h) { return rados_cluster_stat(h, &out); }); },
      [](Napi::Env env, rados_cluster_stat_t& s) {
        auto out = Napi::Object::New(env);
        out.Set("kb", Napi::BigInt::New(env, s.kb));
        out.Set("kbUsed", Napi::BigInt::New(env, s.kb_used));
        out.Set("kbAvailable", Napi::BigInt::New(env, s.kb_avail));
        out.Set("objects", Napi::BigInt::New(env, s.num_objects));
        return out;
      });
}

Napi::Value ClusterWrap::Pools(const Napi::CallbackInfo& info) {
  auto state = state_;
  return async_value<std::vector<std::string>>(info.Env(), "rados_pool_list", {},
      [state](std::vector<std::string>& out) {
        int needed = state->use([](rados_t h) { return rados_pool_list(h, nullptr, 0); });
        if (needed < 0) return needed;
        std::vector<char> buffer(static_cast<size_t>(needed));
        int result = state->use([&](rados_t h) { return rados_pool_list(h, buffer.data(), buffer.size()); });
        if (result < 0) return result;
        for (const char* p = buffer.data(); p < buffer.data() + result && *p; p += std::strlen(p) + 1) out.emplace_back(p);
        return 0;
      }, [](Napi::Env env, std::vector<std::string>& values) {
        auto out = Napi::Array::New(env, values.size());
        for (size_t i = 0; i < values.size(); ++i) out.Set(i, values[i]);
        return out;
      });
}

Napi::Value ClusterWrap::PoolLookup(const Napi::CallbackInfo& info) {
  auto name = string_arg(info, 0, "name");
  auto state = state_;
  return async_value<int64_t>(info.Env(), "rados_pool_lookup", name,
      [state, name](int64_t& out) { out = state->use([&](rados_t h) { return rados_pool_lookup(h, name.c_str()); }); return out < 0 ? static_cast<int>(out) : 0; },
      [](Napi::Env env, int64_t& value) { return Napi::BigInt::New(env, value); });
}

Napi::Value ClusterWrap::PoolReverseLookup(const Napi::CallbackInfo& info) {
  int64_t id = int64_arg(info[0], "id");
  auto state = state_;
  return async_value<std::string>(info.Env(), "rados_pool_reverse_lookup", std::to_string(id),
      [state, id](std::string& out) {
        std::vector<char> buffer(256);
        int result = state->use([&](rados_t h) { return rados_pool_reverse_lookup(h, id, buffer.data(), buffer.size()); });
        if (result >= 0) out = buffer.data();
        return result;
      }, [](Napi::Env env, std::string& value) { return Napi::String::New(env, value); });
}

Napi::Value ClusterWrap::PoolCreate(const Napi::CallbackInfo& info) {
  auto name = string_arg(info, 0, "name"); auto state = state_;
  return async_void(info.Env(), "rados_pool_create", name, [state, name] { return state->use([&](rados_t h) { return rados_pool_create(h, name.c_str()); }); });
}

Napi::Value ClusterWrap::PoolDelete(const Napi::CallbackInfo& info) {
  auto name = string_arg(info, 0, "name"); auto state = state_;
  return async_void(info.Env(), "rados_pool_delete", name, [state, name] { return state->use([&](rados_t h) { return rados_pool_delete(h, name.c_str()); }); });
}

Napi::Value ClusterWrap::OpenIoContext(const Napi::CallbackInfo& info) {
  auto pool = string_arg(info, 0, "pool");
  rados_ioctx_t handle = nullptr;
  int result = state_->use([&](rados_t h) { return rados_ioctx_create(h, pool.c_str(), &handle); });
  if (result < 0) { throw_ceph(info.Env(), result, "rados_ioctx_create", pool); return info.Env().Undefined(); }
  state_->add_child();
  auto io = std::make_shared<IoState>(handle, [cluster = state_](rados_ioctx_t value) { rados_ioctx_destroy(value); cluster->remove_child(); return 0; });
  return IoContextWrap::NewInstance(info.Env(), std::move(io), state_, pool);
}

Napi::Value ClusterWrap::Command(const Napi::CallbackInfo& info) {
  auto kind = string_arg(info, 0, "kind");
  auto target = info.Length() > 1 && info[1].IsString() ? info[1].As<Napi::String>().Utf8Value() : std::string();
  auto commands = js_strings(info[2], "commands");
  auto input = info.Length() > 3 ? js_buffer(info[3], "input") : std::vector<uint8_t>{};
  auto state = state_;
  return async_value<CommandResult>(info.Env(), "rados_" + kind + "_command", target,
      [state, kind, target, commands, input](CommandResult& result) {
        std::vector<const char*> pointers; pointers.reserve(commands.size());
        for (const auto& command : commands) pointers.push_back(command.c_str());
        char* out = nullptr; size_t out_len = 0; char* status = nullptr; size_t status_len = 0;
        int rc = state->use([&](rados_t h) {
          if (kind == "mon") return target.empty()
              ? rados_mon_command(h, pointers.data(), pointers.size(), reinterpret_cast<const char*>(input.data()), input.size(), &out, &out_len, &status, &status_len)
              : rados_mon_command_target(h, target.c_str(), pointers.data(), pointers.size(), reinterpret_cast<const char*>(input.data()), input.size(), &out, &out_len, &status, &status_len);
          if (kind == "mgr") return target.empty()
              ? rados_mgr_command(h, pointers.data(), pointers.size(), reinterpret_cast<const char*>(input.data()), input.size(), &out, &out_len, &status, &status_len)
              : rados_mgr_command_target(h, target.c_str(), pointers.data(), pointers.size(), reinterpret_cast<const char*>(input.data()), input.size(), &out, &out_len, &status, &status_len);
          if (kind == "osd") return rados_osd_command(h, std::stoi(target), pointers.data(), pointers.size(), reinterpret_cast<const char*>(input.data()), input.size(), &out, &out_len, &status, &status_len);
          if (kind == "pg") return rados_pg_command(h, target.c_str(), pointers.data(), pointers.size(), reinterpret_cast<const char*>(input.data()), input.size(), &out, &out_len, &status, &status_len);
          return -EINVAL;
        });
        if (out) { result.output.assign(out, out + out_len); rados_buffer_free(out); }
        if (status) { result.status.assign(status, status_len); rados_buffer_free(status); }
        return rc;
      }, [](Napi::Env env, CommandResult& value) {
        auto out = Napi::Object::New(env);
        out.Set("output", Napi::Buffer<uint8_t>::Copy(env, value.output.data(), value.output.size()));
        out.Set("status", value.status);
        return out;
      });
}

Napi::Value ClusterWrap::PingMonitor(const Napi::CallbackInfo& info) {
  auto monitor = string_arg(info, 0, "monitor"); auto state = state_;
  return async_value<std::string>(info.Env(), "rados_ping_monitor", monitor,
      [state, monitor](std::string& out) { char* data = nullptr; size_t len = 0; int rc = state->use([&](rados_t h) { return rados_ping_monitor(h, monitor.c_str(), &data, &len); }); if (data) { out.assign(data, len); rados_buffer_free(data); } return rc; },
      [](Napi::Env env, std::string& value) { return Napi::String::New(env, value); });
}

Napi::Value ClusterWrap::WaitForLatestOsdMap(const Napi::CallbackInfo& info) {
  auto state = state_; return async_void(info.Env(), "rados_wait_for_latest_osdmap", {}, [state] { return state->use([](rados_t h) { return rados_wait_for_latest_osdmap(h); }); });
}

Napi::Value ClusterWrap::BlocklistAdd(const Napi::CallbackInfo& info) {
  auto address = string_arg(info, 0, "address"); uint32_t expire = static_cast<uint32_t>(uint64_arg(info[1], "expireSeconds")); auto state = state_;
  return async_void(info.Env(), "rados_blocklist_add", address, [state, address, expire] { return state->use([&](rados_t h) { return rados_blocklist_add(h, const_cast<char*>(address.c_str()), expire); }); });
}

Napi::Value ClusterWrap::ServiceRegister(const Napi::CallbackInfo& info) {
  auto service = string_arg(info, 0, "service"); auto daemon = string_arg(info, 1, "daemon"); auto dictionary = js_buffer(info[2], "metadata"); auto state = state_;
  return async_void(info.Env(), "rados_service_register", service + "/" + daemon, [state, service, daemon, dictionary] { return state->use([&](rados_t h) { return rados_service_register(h, service.c_str(), daemon.c_str(), reinterpret_cast<const char*>(dictionary.data())); }); });
}

Napi::Value ClusterWrap::ServiceUpdateStatus(const Napi::CallbackInfo& info) {
  auto dictionary = js_buffer(info[0], "status"); auto state = state_;
  return async_void(info.Env(), "rados_service_update_status", {}, [state, dictionary] { return state->use([&](rados_t h) { return rados_service_update_status(h, reinterpret_cast<const char*>(dictionary.data())); }); });
}

Napi::Value ClusterWrap::MonitorLog(const Napi::CallbackInfo& info) {
  auto level = string_arg(info, 0, "level");
  uint32_t queue = static_cast<uint32_t>(uint64_arg(info[1], "queueSize"));
  if (info.Length() < 3 || !info[2].IsFunction()) throw Napi::TypeError::New(info.Env(), "callback must be a function");
  return MonitorLogWrap::Create(info.Env(), state_, level, queue, info[2].As<Napi::Function>());
}

}  // namespace nl
