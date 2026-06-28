# Troubleshooting Master Not Detecting Slave

## Current Status

**T-Display Slave (COM4):**
- ✅ ESP-NOW initialized successfully
- ✅ Receiving timer broadcasts from master
- ✅ Time syncing correctly (883s = 14:43)
- ✅ Round syncing correctly
- ✅ Sending sync beacons every 3 seconds
- ✅ MAC Address: 5C:01:3B:08:FA:FC

**Master CYD:**
- Shows "M:0S" (not detecting slave)

## Why Master Might Not Detect Slave

The master only tracks slaves when they send **control commands**. The slave is sending sync beacons, but the master's code might be filtering them out.

### Check Master's Serial Monitor

Connect to your CYD master at 115200 baud and look for:

**Expected messages:**
```
Master received command from slave 5C:01:3B:08:FA:FC: 1
New slave connected: 5C:01:3B:08:FA:FC (Total slaves: 1)
```

**If you DON'T see these messages**, the master is not receiving the slave's beacons.

## Solution Options

### Option 1: Restart Master Device (Easiest)

Sometimes ESP-NOW needs both devices to initialize fresh:

1. Power off both devices
2. Power on master first, wait 5 seconds
3. Power on T-Display slave
4. Wait 10 seconds
5. Check master display for "M:1S"

### Option 2: Check Master Configuration

Verify master is in Master mode:
1. Tap gear icon on master
2. Enter config mode
3. Check "Device Sync Mode" = **Master (Controls Others)**
4. Save and reboot if needed

### Option 3: Master Code Modification

The master might be filtering commands with slaveId=99. You can modify the master code to accept these sync beacons.

In the master's `handleControlCommandMessage()` function, ensure it processes commands from all slave IDs, not just specific ones.

### Option 4: Use Real Button on T-Display

The T-Display has two physical buttons:
- GPIO 0 (left button)
- GPIO 35 (right button)

I can add code to make these send real START/PAUSE commands that the master will definitely recognize.

## Debug: What Master Should See

When the slave sends sync beacons, the master should receive ESP-NOW messages with:
- Message Type: 2 (MSG_CONTROL_CMD)
- Command: 1 (CMD_START_PAUSE)
- Slave ID: 99

The master's ESP-NOW receive callback should log these.

## Verification Steps

1. **Check both devices are on same WiFi channel**
   - ESP-NOW uses WiFi radio, channel must match
   - Usually auto-negotiated, but can conflict

2. **Check distance**
   - Move devices closer (within 3 meters)
   - Eliminate obstacles between them

3. **Check master's NVS/Preferences**
   - Master might have stale slave tracking data
   - Factory reset master if needed

4. **Monitor both serial outputs simultaneously**
   - Master should show "received command"
   - Slave should show "sync beacon sent"
   - If slave sends but master doesn't receive, might be channel/frequency issue

## Current Slave Settings

**Sync Beacon Details:**
- Sent every 3 seconds
- Message type: MSG_CONTROL_CMD (2)
- Command: CMD_START_PAUSE (1)
- Slave ID: 99 (special beacon ID)
- Broadcast to: FF:FF:FF:FF:FF:FF

**If you need me to change any of these**, let me know and I can adjust:
- Beacon frequency
- Command type used
- Slave ID number
- Add physical button support

## Next Steps

1. Check master's serial monitor for received messages
2. Try restarting both devices (master first)
3. Let me know what you see in master's serial output
4. I can add button support if needed for testing

---

**Your slave is working correctly and sending sync beacons!** The issue is on the master side - either filtering, timing, or configuration.
