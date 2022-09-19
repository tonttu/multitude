/* Copyright (C) 2007-2022: Multi Touch Oy, Helsinki University of Technology
 * and others.
 *
 * This file is licensed under GNU Lesser General Public License (LGPL),
 * version 2.1. The LGPL conditions can be found in file "LGPL.txt" that is
 * distributed with this source package or obtained from the GNU organization
 * (www.gnu.org).
 *
 */

#include "DisplayConfigWin.hpp"

#include <QRegularExpression>

namespace Luminous
{
  QString DisplayConfigWin::gdiDeviceToId(const QString & sourceGdiDeviceName)
  {
    if (sourceGdiDeviceName.startsWith("\\\\.\\DISPLAY")) {
      return sourceGdiDeviceName.mid(11);
    }
    return sourceGdiDeviceName;
  }

  QString DisplayConfigWin::cleanInstanceId(const QString & adapterDevicePath)
  {
    QString tmp = adapterDevicePath;
    if (tmp.startsWith("\\\\?\\")) {
      tmp = tmp.mid(4);
    }
    tmp.replace("#", "\\");
    tmp.replace(QRegularExpression("\\\\\\{[a-z0-9-]+\\}$"), "");
    return tmp.toUpper();
  }

  DisplayConfigWin DisplayConfigWin::currentConfig()
  {
    DisplayConfigWin cfg;

    uint32_t flags = QDC_ALL_PATHS;
    uint32_t numPathElements = 0;
    uint32_t numModeInfoElements = 0;
    auto err = GetDisplayConfigBufferSizes(flags, &numPathElements, &numModeInfoElements);
    if (err != ERROR_SUCCESS)
      throw std::runtime_error("GetDisplayConfigBufferSizes");

    std::vector<DISPLAYCONFIG_PATH_INFO> pathInfo(numPathElements);
    std::vector<DISPLAYCONFIG_MODE_INFO> modeInfo(numModeInfoElements);

    if (pathInfo.empty()) {
      return cfg;
    }

    err = QueryDisplayConfig(flags, &numPathElements, pathInfo.data(),
                             &numModeInfoElements, modeInfo.data(), nullptr);
    if (err != ERROR_SUCCESS) {
      throw std::runtime_error("QueryDisplayConfig");
    }

    for (const DISPLAYCONFIG_PATH_INFO & path: pathInfo) {
      if (!path.targetInfo.targetAvailable) continue;

      Output & output = cfg.findOrCreate(path.sourceInfo.adapterId, path.sourceInfo.id);

      if (output.adapterDevicePath.isEmpty()) {
        DISPLAYCONFIG_ADAPTER_NAME  namereq;
        namereq.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_ADAPTER_NAME;
        namereq.header.adapterId = path.targetInfo.adapterId;
        namereq.header.id = 0;
        namereq.header.size = sizeof(namereq);

        auto err = DisplayConfigGetDeviceInfo(reinterpret_cast<DISPLAYCONFIG_DEVICE_INFO_HEADER*>(&namereq));
        if (err != ERROR_SUCCESS)
          throw std::runtime_error("DisplayConfigGetDeviceInfo");
        output.adapterDevicePath = QString::fromWCharArray(namereq.adapterDevicePath);
      }

      if (output.sourceGdiDeviceName.isEmpty()) {
        DISPLAYCONFIG_SOURCE_DEVICE_NAME namereq;
        namereq.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
        namereq.header.adapterId = path.sourceInfo.adapterId;
        namereq.header.id = path.sourceInfo.id;
        namereq.header.size = sizeof(namereq);

        auto err = DisplayConfigGetDeviceInfo(reinterpret_cast<DISPLAYCONFIG_DEVICE_INFO_HEADER*>(&namereq));
        if (err != ERROR_SUCCESS)
          throw std::runtime_error("DisplayConfigGetDeviceInfo");
        output.sourceGdiDeviceName = QString::fromWCharArray(namereq.viewGdiDeviceName);
      }

      if (path.sourceInfo.modeInfoIdx < modeInfo.size()) {
        DISPLAYCONFIG_MODE_INFO & m = modeInfo[path.sourceInfo.modeInfoIdx];
        if (m.infoType != DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE) {
          throw std::runtime_error("DISPLAYCONFIG_MODE_INFO was expected to be DISPLAYCONFIG_MODE_INFO_TYPE_SOURCE");
        }
        output.rect = QRect(m.sourceMode.position.x, m.sourceMode.position.y,
                            m.sourceMode.width, m.sourceMode.height);
      }

      Target target;
      target.info = path.targetInfo;
      {
        DISPLAYCONFIG_TARGET_DEVICE_NAME namereq;
        namereq.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        namereq.header.adapterId = path.targetInfo.adapterId;
        namereq.header.id = path.targetInfo.id;
        namereq.header.size = sizeof(namereq);

        auto err = DisplayConfigGetDeviceInfo(reinterpret_cast<DISPLAYCONFIG_DEVICE_INFO_HEADER*>(&namereq));
        if (err != ERROR_SUCCESS)
          throw std::runtime_error("DisplayConfigGetDeviceInfo");

        target.devicePath = QString::fromWCharArray(namereq.monitorDevicePath);
        target.friendlyDeviceName = QString::fromWCharArray(namereq.monitorFriendlyDeviceName);

        if ((path.flags & DISPLAYCONFIG_PATH_ACTIVE) &&
            (path.targetInfo.statusFlags & DISPLAYCONFIG_TARGET_IN_USE) &&
            output.activeTarget.devicePath.isEmpty()) {
          output.activeTarget = target;
        }
      }

      output.targets.push_back(target);

      if (path.flags & DISPLAYCONFIG_PATH_ACTIVE) {
        output.active = true;
      }

      if (output.preferredTargetResolution.width() <= 0) {
        DISPLAYCONFIG_TARGET_PREFERRED_MODE req;
        req.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_PREFERRED_MODE;
        req.header.adapterId = path.targetInfo.adapterId;
        req.header.id = path.targetInfo.id;
        req.header.size = sizeof(req);

        auto err = DisplayConfigGetDeviceInfo(reinterpret_cast<DISPLAYCONFIG_DEVICE_INFO_HEADER*>(&req));
        if (err != ERROR_SUCCESS)
          throw std::runtime_error("DisplayConfigGetDeviceInfo");

        output.preferredTargetResolution = QSize(req.width, req.height);
        output.preferredTargetMode = req.targetMode;
      }
    }

    return cfg;
  }

  DisplayConfigWin::Output & DisplayConfigWin::findOrCreate(DisplayConfigWin::AdapterId adapterId, uint32_t id)
  {
    for (Output & output: outputs) {
      if (output.adapterId == adapterId && output.id == id)
        return output;
    }
    outputs.emplace_back();
    Output & output = outputs.back();
    output.adapterId = adapterId;
    output.id = id;
    return output;
  }

  const DisplayConfigWin::Output * DisplayConfigWin::find(const QString & gdiId) const
  {
    for (const Output & output: outputs) {
      if (gdiDeviceToId(output.sourceGdiDeviceName) == gdiId)
        return &output;
    }
    return nullptr;
  }

} // namespace Luminous
