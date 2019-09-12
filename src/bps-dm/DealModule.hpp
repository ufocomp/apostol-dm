/*++

Program name:

  deal-module

Module Name:

  DealModule.hpp

Notices:

  Bitcoin Payment Service (Deal Module)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_APOSTOL_HPP
#define APOSTOL_APOSTOL_HPP

#include "../../version.h"
//----------------------------------------------------------------------------------------------------------------------

#define APP_VERSION      AUTO_VERSION
#define APP_VER          APP_NAME "/" APP_VERSION
//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "Modules.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace DealModule {

        class CDealModule: public CApplication {
        protected:

            void ParseCmdLine() override;
            void ShowVersioInfo() override;

        public:

            CDealModule(int argc, char *const *argv): CApplication(argc, argv) {
                CreateModule(this);
            };

            ~CDealModule() override = default;

            static class CDealModule *Create(int argc, char *const *argv) {
                return new CDealModule(argc, argv);
            };

            inline void Destroy() override { delete this; };

            void Run() override;

        };
    }
}

using namespace Apostol::DealModule;
}

#endif //APOSTOL_APOSTOL_HPP

