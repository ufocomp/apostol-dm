/*++

Library name:

  apostol-core

Module Name:

  Workers.hpp

Notices:

  Add-ons: Workers

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_WORKERS_HPP
#define APOSTOL_WORKERS_HPP
//----------------------------------------------------------------------------------------------------------------------

#include "WebService/WebService.hpp"
//----------------------------------------------------------------------------------------------------------------------

static inline void CreateWorkers(CModuleProcess *AProcess) {
    CWebService::CreateModule(AProcess);
}

#endif //APOSTOL_WORKERS_HPP
