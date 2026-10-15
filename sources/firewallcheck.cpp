/****************************************************************************
**
** This file is part of LAN Messenger.
**
** Copyright (c) LAN Messenger Authors.
** 
** LAN Messenger is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** LAN Messenger is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with LAN Messenger.  If not, see <http://www.gnu.org/licenses/>.
**
****************************************************************************/

#include <QCoreApplication>
#include <QDir>
#include <QMetaObject>
#include <QThread>
#include "firewallcheck.h"

#ifdef Q_OS_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <objbase.h>
#include <oleauto.h>
#include <netfw.h>

#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#endif

namespace {

//  Block rules win over allow rules in Windows Firewall, so one matching block rule is enough.
bool ruleBlocksExe(INetFwRule* rule, const QString& exe, long activeProfiles) {
    VARIANT_BOOL enabled = VARIANT_FALSE;
    if(FAILED(rule->get_Enabled(&enabled)) || enabled == VARIANT_FALSE)
        return false;
    NET_FW_ACTION action = NET_FW_ACTION_ALLOW;
    if(FAILED(rule->get_Action(&action)) || action != NET_FW_ACTION_BLOCK)
        return false;
    NET_FW_RULE_DIRECTION direction = NET_FW_RULE_DIR_OUT;
    if(FAILED(rule->get_Direction(&direction)) || direction != NET_FW_RULE_DIR_IN)
        return false;
    long profiles = 0;
    if(SUCCEEDED(rule->get_Profiles(&profiles)) && !(profiles & activeProfiles))
        return false;

    BSTR application = nullptr;
    if(FAILED(rule->get_ApplicationName(&application)) || !application)
        return false;
    const QString name = QString::fromWCharArray(application, int(SysStringLen(application)));
    SysFreeString(application);
    return QDir::fromNativeSeparators(name).compare(QDir::fromNativeSeparators(exe), Qt::CaseInsensitive) == 0;
}

bool scanRules(INetFwPolicy2* policy, const QString& exe) {
    long active = 0;
    if(FAILED(policy->get_CurrentProfileTypes(&active)))
        return false;

    //  With the firewall switched off for every active profile, a leftover block rule does nothing.
    bool firewallOn = false;
    for(const long profile : { long(NET_FW_PROFILE2_DOMAIN), long(NET_FW_PROFILE2_PRIVATE), long(NET_FW_PROFILE2_PUBLIC) }) {
        VARIANT_BOOL on = VARIANT_FALSE;
        if((active & profile) && SUCCEEDED(policy->get_FirewallEnabled(NET_FW_PROFILE_TYPE2(profile), &on)) && on != VARIANT_FALSE)
            firewallOn = true;
    }
    if(!firewallOn)
        return false;

    INetFwRules* rules = nullptr;
    if(FAILED(policy->get_Rules(&rules)) || !rules)
        return false;

    bool found = false;
    IUnknown* enumUnknown = nullptr;
    IEnumVARIANT* rulesEnum = nullptr;
    if(SUCCEEDED(rules->get__NewEnum(&enumUnknown)) && enumUnknown) {
        enumUnknown->QueryInterface(IID_PPV_ARGS(&rulesEnum));
        enumUnknown->Release();
    }
    if(rulesEnum) {
        VARIANT item;
        VariantInit(&item);
        ULONG fetched = 0;
        while(!found && rulesEnum->Next(1, &item, &fetched) == S_OK && fetched == 1) {
            if(item.vt == VT_DISPATCH && item.pdispVal) {
                INetFwRule* rule = nullptr;
                if(SUCCEEDED(item.pdispVal->QueryInterface(IID_PPV_ARGS(&rule))) && rule) {
                    found = ruleBlocksExe(rule, exe, active);
                    rule->Release();
                }
            }
            VariantClear(&item);
        }
        rulesEnum->Release();
    }
    rules->Release();
    return found;
}

bool isBlockedByRule(const QString& exe) {
    const HRESULT init = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool blocked = false;
    INetFwPolicy2* policy = nullptr;
    if(SUCCEEDED(CoCreateInstance(__uuidof(NetFwPolicy2), nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&policy))) && policy) {
        blocked = scanRules(policy, exe);
        policy->Release();
    }
    if(SUCCEEDED(init))
        CoUninitialize();
    return blocked;
}

}   // namespace

#endif  // Q_OS_WIN

lmFirewallCheck::lmFirewallCheck(QObject* parent) : QObject(parent) {
}

lmFirewallCheck::~lmFirewallCheck(void) {
    if(m_thread) {
        m_thread->wait();   // the thread posts back to this object
        delete m_thread;
    }
}

void lmFirewallCheck::start(void) {
#ifdef Q_OS_WIN
    if(m_thread && m_thread->isRunning())
        return;
    delete m_thread;
    const QString exe = QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
    m_thread = QThread::create([this, exe]() {
        if(isBlockedByRule(exe))
            QMetaObject::invokeMethod(this, [this]() { emit blocked(); }, Qt::QueuedConnection);
    });
    m_thread->start();
#endif
}
