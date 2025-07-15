# 🔍 Apollo Backend API Requirements Analysis

## 🎯 Critical Question: Which Features Require Apollo Backend?

You're absolutely right to question this! We need to determine which features require Apollo server APIs vs. which are pure client-side Moonlight enhancements.

## 📋 Feature Classification

### ✅ **Confirmed Apollo-Only Features** (From Your Analysis)
| Feature | Apollo Required | Evidence |
|---------|----------------|----------|
| **Clipboard Sync** | ✅ YES | Uses `/actions/clipboard` endpoint |
| **Server Commands** | ✅ YES | Requires `server_cmd` permission |
| **OTP Pairing** | ✅ YES | Uses `&otpauth=` parameter |
| **Permission Viewing** | ✅ YES | Reads Apollo permission flags |

### ❓ **Unknown/Need Research** (Your Remaining Features)
| Feature | Apollo Required? | Research Needed |
|---------|-----------------|-----------------|
| **Fractional Refresh Rate** | ❓ UNKNOWN | Check Artemis Android implementation |
| **Resolution Scaling** | ❓ UNKNOWN | Check if client-side or server negotiation |
| **Virtual Display Control** | ❓ UNKNOWN | Check if Apollo API or client setting |
| **App Ordering** | ❓ UNKNOWN | Check if server-side or client preference |
| **Input Only Mode** | ❓ UNKNOWN | Not yet implemented in Android |

## 🔬 Research Strategy

We need to analyze the Artemis Android source code to understand:

### 1. Fractional Refresh Rate Implementation
**Key Questions**:
- Does it modify the GameStream protocol negotiation?
- Does it send custom refresh rate to Apollo server?
- Or is it purely client-side frame timing?

**Files to Check in Artemis Android**:
```
- StreamConfiguration.java (refresh rate negotiation)
- NvHTTP.java (server communication)
- StreamingPreferences.java (client settings)
- Any Apollo-specific refresh rate endpoints
```

### 2. Resolution Scaling Implementation  
**Key Questions**:
- Does it change the resolution negotiation with server?
- Is it client-side scaling of the received stream?
- Does Apollo need to know about the scaling?

**Files to Check**:
```
- StreamConfiguration.java (resolution negotiation)
- Video decoder/renderer classes
- Apollo-specific resolution endpoints
```

### 3. Virtual Display Control
**Key Questions**:
- Does this control Apollo server's virtual display creation?
- Is it a server-side setting or client preference?
- Does it use Apollo APIs to manage displays?

### 4. App Ordering Without Compatibility Mode
**Key Questions**:
- Does this require Apollo server to store custom order?
- Is it client-side sorting of the app list?
- Does it use Apollo APIs for app metadata?

## 🎯 Immediate Action Plan

### Step 1: Research Artemis Android Implementation
Before implementing any of the "unknown" features, we need to:

1. **Examine Artemis Android source** for each feature
2. **Identify Apollo API dependencies** vs client-side logic
3. **Document the actual implementation approach**
4. **Update our roadmap** based on findings

### Step 2: Categorize Features Properly
After research, update our feature list:

**Apollo-Dependent Features** (Require Apollo server):
- Clipboard Sync ✅
- Server Commands ✅  
- OTP Pairing ✅
- Permission Viewing ✅
- [Others TBD based on research]

**Client-Side Features** (Work with any GameStream server):
- [TBD based on research]

**Hybrid Features** (Enhanced with Apollo, basic with GameStream):
- [TBD based on research]

## 🚨 Risk Assessment

### High Risk: Assuming Client-Side Implementation
If we implement features as "client-side" when they actually require Apollo APIs:
- ❌ Features won't work with Apollo servers
- ❌ Wasted development time
- ❌ Incompatible with Android Artemis behavior

### Low Risk: Research First Approach
If we research the Android implementation first:
- ✅ Accurate implementation matching Android behavior
- ✅ Proper Apollo API integration
- ✅ Compatible feature set
- ✅ No wasted effort

## 🔍 Research Questions for Each Feature

### Fractional Refresh Rate
1. Does Artemis Android send custom refresh rates to Apollo server?
2. Is there an Apollo API endpoint for refresh rate negotiation?
3. How does it differ from standard GameStream refresh rate handling?
4. Does Apollo server need to support fractional rates?

### Resolution Scaling  
1. Does Artemis Android negotiate custom resolutions with Apollo?
2. Is scaling done client-side or server-side?
3. Are there Apollo-specific resolution APIs?
4. How does it interact with virtual display creation?

### Virtual Display Control
1. Does this control Apollo server's display management?
2. Are there Apollo APIs for virtual display creation/destruction?
3. Is it a server setting or client preference?
4. How does it integrate with streaming session setup?

### App Ordering
1. Does Apollo server store custom app order?
2. Are there Apollo APIs for app metadata/ordering?
3. Is it client-side preference or server-side data?
4. How does it sync across multiple clients?

## 📋 Next Steps

### Immediate (Before Any Implementation)
1. **Research Artemis Android source code** for each unknown feature
2. **Document Apollo API dependencies** for each feature
3. **Create accurate implementation plan** based on findings
4. **Update roadmap** with correct Apollo vs client-side classification

### After Research
1. **Prioritize Apollo-dependent features** (require Apollo server for testing)
2. **Implement client-side features first** (can test immediately)
3. **Plan Apollo API integration** for server-dependent features

## 🎯 Expected Outcome

After this research, we'll have:
- ✅ **Accurate feature classification** (Apollo vs client-side)
- ✅ **Proper implementation approach** matching Android behavior
- ✅ **Realistic timeline** based on actual requirements
- ✅ **Compatible feature set** with Artemis ecosystem

This research phase is **critical** to avoid implementing features incorrectly and ensure compatibility with the broader Artemis ecosystem.

## 🚀 Research Priority Order

1. **Fractional Refresh Rate** (Most likely to be client-side)
2. **Resolution Scaling** (Could be hybrid)
3. **Virtual Display Control** (Likely Apollo-dependent)
4. **App Ordering** (Likely Apollo-dependent)

Let's start with analyzing the Artemis Android implementation for these features!