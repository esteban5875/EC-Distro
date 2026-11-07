package core;

import (
    "crypto/aes"
    "crypto/cipher"
    "crypto/rand"
    "crypto/sha256"
    "encoding/hex"
    "encoding/json"
    "fmt"
    "io"
    "os"
    "path/filepath"
    "github.com/joho/godotenv"
)

type indexEntry struct {
	TarName   string `json "tar_name"`
	TarHeader string `json: "tar_header"`
}

const INDEX_ERR = -1
const INDEX_SUCCESS = 0

func CreateIndex() int {
	var err error

	key := make([]byte, 32)
	_, err = rand.Read(key)
	
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error generating random bytes: %v\n", err)
		return INDEX_ERR
	}

	keyString := hex.EncodeToString(key)

	file, err := os.OpenFile(".env", os.O_APPEND|os.O_WRONLY|os.O_CREATE, 0644)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error opening .env file: %v\n", err)
		return INDEX_ERR
	}
	defer file.Close()

	envLine := fmt.Sprintf("\nENC_KEY=%s\n", keyString)
	_, err = file.WriteString(envLine)

	if err != nil {
		fmt.Fprintf(os.Stderr, "Error writing .env file: %v\n", err)
	}

	if err := godotenv.Load(); err != nil {	
		fmt.Printf("Error getting .env file: %s.\n", err)
		return INDEX_ERR;
	}

	rawSecretKey := os.Getenv("ENC_KEY")
	if rawSecretKey == "" {
		fmt.Println("ENC_KEY is empty or missing.")
		return INDEX_ERR
	}

	hash := sha256.Sum256([]byte(rawSecretKey)) //Force key to be 32 bytes long
	secretKey := hash[:]

	if len(secretKey) != 32 {
		fmt.Printf("ENC_KEY must be exactly 32 bytes long.")
		return INDEX_ERR;
	}

	tarballs, err := GetAll(TAR_DIR, true)

	if err != nil {
		fmt.Printf("Error getting tarballs: %s.", err.Error())
		return INDEX_ERR;
	}

	var indexEntries []indexEntry

	for i := 0; i < len(tarballs); i++ {
		tarRoute := filepath.Join(TAR_DIR, tarballs[i])
		tarHeader, err := ReadTarballHeader(tarRoute)

		if err != nil {
			fmt.Printf("Error fetching tarball header: %s.", err.Error())
			return INDEX_ERR;
		}

		newEntry := indexEntry{
			TarName: tarballs[i],
			TarHeader: tarHeader,
		}

		indexEntries = append(indexEntries, newEntry)
	}

	jsonData, err := json.Marshal(indexEntries)

	if err != nil {
		fmt.Printf("Error serializing index: %s.\n", err.Error())
		return INDEX_ERR;
	}

	block, err := aes.NewCipher(secretKey)
	if err != nil {
		fmt.Printf("Error creating cifer: %s\n", err.Error())
		return INDEX_ERR;
	}

	aesGCM, err := cipher.NewGCM(block)
	if err != nil {
		fmt.Printf("Error configuring GCM: %s.\n", err.Error())
		return INDEX_ERR;
	}

	nonce := make([]byte, aesGCM.NonceSize())
	if _, err = io.ReadFull(rand.Reader, nonce); err != nil {
		fmt.Printf("Error generating nonce: %s.\n", err.Error())
		return INDEX_ERR;
	}

	encryptedData := aesGCM.Seal(nonce, nonce, jsonData, nil)

	indexDir := filepath.Join(REPO_DIR, "index")
	err 	  = os.WriteFile(indexDir, encryptedData, 0600)

	if err != nil {
		fmt.Printf("Error saving encrypted index: %s.\n", err.Error())
		return INDEX_ERR;
	}

	fmt.Printf("Index created.\n")
	return INDEX_SUCCESS
}

//func ReadIndex(); //TODO
//func UpdateIndex(); //TODO