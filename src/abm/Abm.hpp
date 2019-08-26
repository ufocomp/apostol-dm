/*++

Programm name:

  abc

Module Name:

  Abc.hpp

Notices:

  Apostol Bitcoin

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

    namespace AbcModule {

        class CAbcModule: public CApplication {
        protected:

            void ParseCmdLine() override;
            void ShowVersioInfo() override;

        public:

            CAbcModule(int argc, char *const *argv): CApplication(argc, argv) {
                CreateModule(this);
            };

            ~CAbcModule() override = default;

            static class CAbcModule *Create(int argc, char *const *argv) {
                return new CAbcModule(argc, argv);
            };

            inline void Destroy() override { delete this; };

            void Run() override;

        };
    }
}

using namespace Apostol::AbcModule;
}

#endif //APOSTOL_APOSTOL_HPP

