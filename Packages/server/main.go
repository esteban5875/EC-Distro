package main

import (
	"fmt"
	"log"
	"github.com/google/uuid"
	"server/core"
)

func main ()  {
	var compilation = core.Compile()

	if compilation == core.COMPILE_ERR {
		log.Println("Compilation failed.")
		return;
	}

    executables, err := core.GetAll(core.BIN_DIR)
	
	if err != nil {
		log.Printf("Error reading BIN_DIR (%s): %v\n", core.BIN_DIR, err.Error())
		return;
	}

	for i := 0; i < len(executables); i++ {
		var headerID = uuid.New().String();
		create := core.CreateTarball(core.TAR_DIR, executables[i], string(headerID));

		if create != nil {
			return;
		}
	}

	fmt.Printf("Executables found: %v\n", executables)

	var createIndex = core.CreateIndex();

	if createIndex != core.INDEX_SUCCESS {
		log.Println("Error creating index.")
		return;
	}

	core.InitServer(core.TAR_DIR)
}