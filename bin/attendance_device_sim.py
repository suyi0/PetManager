#!/usr/bin/env python3
"""模拟考勤机：无真机时验证设备协议（HMAC 验签、防重放、幂等、时钟规则）。

用法示例：
  python3 bin/attendance_device_sim.py --device-key dev01 --secret <hex> heartbeat
  python3 bin/attendance_device_sim.py --device-key dev01 --secret <hex> punch --attendance-no 1
  python3 bin/attendance_device_sim.py --device-key dev01 --secret <hex> punch \
      --attendance-no 1 --punched-at "2026-07-10 08:55:30" --verify-mode face
  python3 bin/attendance_device_sim.py --device-key dev01 --secret <hex> suite --attendance-no 1

suite 场景（对应设计 §13 测试计划）：正常打卡 / 幂等重推（同 event_id）/
nonce 重放 / 错误签名 / 未来时间 / 未知考勤号 / 心跳。
"""

import argparse
import hashlib
import hmac
import json
import secrets
import sys
import time
import urllib.error
import urllib.request

PUNCH_PATH = "/api/device/attendance/punch"
PUNCH_BATCH_PATH = "/api/device/attendance/punches"
HEARTBEAT_PATH = "/api/device/attendance/heartbeat"
SYNC_PULL_PATH = "/api/device/attendance/person-sync/pull"
SYNC_ACK_PATH = "/api/device/attendance/person-sync/ack"


def sign(method: str, path: str, timestamp: str, nonce: str, body: str, secret: str) -> str:
    canonical = "\n".join([
        method,
        path,
        timestamp,
        nonce,
        hashlib.sha256(body.encode("utf-8")).hexdigest(),
    ])
    return hmac.new(secret.encode("utf-8"), canonical.encode("utf-8"), hashlib.sha256).hexdigest()


def send(base_url: str, path: str, body: dict, device_key: str, secret: str,
         timestamp: str = None, nonce: str = None, bad_sign: bool = False):
    raw = json.dumps(body, ensure_ascii=False)
    ts = timestamp if timestamp is not None else str(int(time.time()))
    nc = nonce if nonce is not None else secrets.token_hex(16)
    signature = sign("POST", path, ts, nc, raw, secret)
    if bad_sign:
        signature = "0" * len(signature)
    request = urllib.request.Request(
        base_url.rstrip("/") + path,
        data=raw.encode("utf-8"),
        method="POST",
        headers={
            "Content-Type": "application/json",
            "X-Device-Key": device_key,
            "X-Device-Timestamp": ts,
            "X-Device-Nonce": nc,
            "X-Device-Sign": signature,
        },
    )
    try:
        with urllib.request.urlopen(request, timeout=10) as response:
            return response.status, response.read().decode("utf-8", "replace"), nc
    except urllib.error.HTTPError as error:
        return error.code, error.read().decode("utf-8", "replace"), nc
    except urllib.error.URLError as error:
        print(f"连接失败：{error.reason}", file=sys.stderr)
        sys.exit(2)


def now_text() -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S")


def report(label: str, status: int, body: str, expect):
    ok = status in expect if isinstance(expect, (tuple, list, set)) else status == expect
    mark = "PASS" if ok else "FAIL"
    print(f"[{mark}] {label}: HTTP {status} {body.strip()[:200]}")
    return ok


def run_suite(args) -> int:
    base = dict(base_url=args.base_url, device_key=args.device_key, secret=args.secret)
    event_id = f"sim-{secrets.token_hex(6)}"
    punch = {
        "attendance_no": args.attendance_no,
        "punched_at": args.punched_at or now_text(),
        "verify_mode": "face",
        "event_id": event_id,
    }
    passed = []

    status, body, _ = send(path=HEARTBEAT_PATH, body={}, **base)
    passed.append(report("心跳+服务器对时", status, body, 200)
                  and "server_timestamp" in body and "protocol_version" in body)

    status, body, _ = send(path=PUNCH_PATH, body=punch, **base)
    passed.append(report("正常打卡", status, body, 200))

    batch = {
        "events": [
            dict(punch, event_id=f"sim-{secrets.token_hex(6)}"),
            dict(punch, event_id=f"sim-{secrets.token_hex(6)}", attendance_no="no-such-person"),
        ]
    }
    status, body, _ = send(path=PUNCH_BATCH_PATH, body=batch, **base)
    passed.append(report("批量补推(逐条返回结果)", status, body, 200)
                  and "accepted_count" in body and "rejected_count" in body)

    status, body, _ = send(path=PUNCH_PATH, body=punch, **base)
    passed.append(report("幂等重推(同 event_id, 新 nonce)", status, body, 200))

    ts = str(int(time.time()))
    nc = secrets.token_hex(16)
    status, body, _ = send(path=PUNCH_PATH, body=punch, timestamp=ts, nonce=nc, **base)
    status2, body2, _ = send(path=PUNCH_PATH, body=punch, timestamp=ts, nonce=nc, **base)
    passed.append(report("nonce 重放(第二次应 accepted duplicate)", status2, body2, 200)
                  and "duplicate" in body2)

    status, body, _ = send(path=PUNCH_PATH, body=punch, bad_sign=True, **base)
    passed.append(report("错误签名(应 401)", status, body, 401))

    status, body, _ = send(path=PUNCH_PATH, body=punch,
                           timestamp=str(int(time.time()) - 3600), **base)
    passed.append(report("过期时间戳(应 401)", status, body, 401))

    future = dict(punch, event_id=f"sim-{secrets.token_hex(6)}",
                  punched_at=time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(time.time() + 3600)))
    status, body, _ = send(path=PUNCH_PATH, body=future, **base)
    passed.append(report("未来打卡时间(应 4xx)", status, body, (400, 401, 422)))

    unknown = dict(punch, event_id=f"sim-{secrets.token_hex(6)}", attendance_no="no-such-person")
    status, body, _ = send(path=PUNCH_PATH, body=unknown, **base)
    passed.append(report("未知考勤号(应 4xx)", status, body, (400, 401, 422)))

    total, ok = len(passed), sum(1 for item in passed if item)
    print(f"\n{ok}/{total} 场景通过")
    return 0 if ok == total else 1


def main() -> int:
    parser = argparse.ArgumentParser(description="PetManager 考勤机模拟器")
    parser.add_argument("--base-url", default="http://127.0.0.1:8081")
    parser.add_argument("--device-key", required=True)
    parser.add_argument("--secret", required=True, help="设备 HMAC 密钥（创建/轮换时返回的一次性 secret）")
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("heartbeat")

    punch = sub.add_parser("punch")
    punch.add_argument("--attendance-no", required=True)
    punch.add_argument("--punched-at", default=None, help="YYYY-MM-DD HH:MM:SS，缺省为当前时间（离线补推传历史时间）")
    punch.add_argument("--verify-mode", default="face")
    punch.add_argument("--event-id", default=None)

    batch = sub.add_parser("batch", help="批量补推离线打卡")
    batch.add_argument("--attendance-no", required=True)
    batch.add_argument("--count", type=int, default=2)
    batch.add_argument("--punched-at", default=None)

    sync_pull = sub.add_parser("sync-pull", help="领取人员同步任务")
    sync_pull.add_argument("--limit", type=int, default=50)

    sync_ack = sub.add_parser("sync-ack", help="确认一条人员同步任务")
    sync_ack.add_argument("--task-id", type=int, required=True)
    sync_ack.add_argument("--failed", action="store_true")
    sync_ack.add_argument("--message", default="")

    suite = sub.add_parser("suite", help="跑完整协议验证场景")
    suite.add_argument("--attendance-no", required=True)
    suite.add_argument("--punched-at", default=None)

    args = parser.parse_args()
    base = dict(base_url=args.base_url, device_key=args.device_key, secret=args.secret)

    if args.command == "heartbeat":
        status, body, _ = send(path=HEARTBEAT_PATH, body={}, **base)
        print(f"HTTP {status} {body}")
        return 0 if status == 200 else 1
    if args.command == "punch":
        body_json = {
            "attendance_no": args.attendance_no,
            "punched_at": args.punched_at or now_text(),
            "verify_mode": args.verify_mode,
            "event_id": args.event_id or f"sim-{secrets.token_hex(6)}",
        }
        status, body, _ = send(path=PUNCH_PATH, body=body_json, **base)
        print(f"HTTP {status} {body}")
        return 0 if status == 200 else 1
    if args.command == "batch":
        count = max(1, min(args.count, 100))
        body_json = {
            "events": [
                {
                    "attendance_no": args.attendance_no,
                    "punched_at": args.punched_at or now_text(),
                    "verify_mode": "face",
                    "event_id": f"sim-{secrets.token_hex(6)}",
                }
                for _ in range(count)
            ]
        }
        status, body, _ = send(path=PUNCH_BATCH_PATH, body=body_json, **base)
        print(f"HTTP {status} {body}")
        return 0 if status == 200 else 1
    if args.command == "sync-pull":
        status, body, _ = send(path=SYNC_PULL_PATH, body={"limit": args.limit}, **base)
        print(f"HTTP {status} {body}")
        return 0 if status == 200 else 1
    if args.command == "sync-ack":
        body_json = {
            "results": [{
                "task_id": args.task_id,
                "success": not args.failed,
                "message": args.message,
            }]
        }
        status, body, _ = send(path=SYNC_ACK_PATH, body=body_json, **base)
        print(f"HTTP {status} {body}")
        return 0 if status == 200 else 1
    return run_suite(args)


if __name__ == "__main__":
    sys.exit(main())
