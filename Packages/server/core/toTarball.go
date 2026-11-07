package core;


import (
	"archive/tar"
	"compress/gzip"
	"errors"
	"fmt"
	"io"
	"os"
	"path/filepath"
)


func CreateTarball(targetDir, executablePath, headerID string) error {
    execFile, err := os.Open(executablePath)
    if err != nil {
        return fmt.Errorf("failed to open executable: %w", err)
    }
    defer execFile.Close()

    info, err := execFile.Stat()
    if err != nil {
        return fmt.Errorf("failed to get file stats: %w", err)
    }

    if !info.Mode().IsRegular() {
        return fmt.Errorf("executable is not a regular file: %s", executablePath)
    }

    if err := os.MkdirAll(targetDir, 0755); err != nil {
        return fmt.Errorf("failed to create target directory: %w", err)
    }

    tarballName := info.Name() + ".tar.gz"
    tarballPath := filepath.Join(targetDir, tarballName)

    tarballFile, err := os.OpenFile(
        tarballPath,
        os.O_CREATE|os.O_WRONLY|os.O_EXCL,
        0666,
    )
    if err != nil {
    	if errors.Is(err, os.ErrExist) {
        	fmt.Printf("Skipping existing tarball: %s\n", tarballPath)
        	return nil
   		}
    	
		return fmt.Errorf("failed to create tarball file: %w", err)
	}

    // Remove incomplete archives if anything fails.
    success := false
    defer func() {
        tarballFile.Close()
        if !success {
            os.Remove(tarballPath)
        }
    }()

    gw := gzip.NewWriter(tarballFile)
    tw := tar.NewWriter(gw)

    header, err := tar.FileInfoHeader(info, info.Name())
    if err != nil {
        return fmt.Errorf("failed to create tar header: %w", err)
    }

    header.Name = info.Name()

    if header.PAXRecords == nil {
        header.PAXRecords = make(map[string]string)
    }

    header.PAXRecords["headerID"] = headerID

    if err := tw.WriteHeader(header); err != nil {
        return fmt.Errorf("failed to write header: %w", err)
    }

    if _, err := io.Copy(tw, execFile); err != nil {
        return fmt.Errorf("failed to copy executable: %w", err)
    }

    if err := tw.Close(); err != nil {
        return fmt.Errorf("failed to close tar writer: %w", err)
    }

    if err := gw.Close(); err != nil {
        return fmt.Errorf("failed to close gzip writer: %w", err)
    }

    if err := tarballFile.Close(); err != nil {
        return fmt.Errorf("failed to close tarball file: %w", err)
    }

    success = true

    fmt.Printf("Successfully created: %s\n", tarballPath)
    return nil
}

func ReadTarballHeader(tarballPath string) (string, error) {
	file, err := os.Open(tarballPath)
	if err != nil {
		return "", fmt.Errorf("failed to open archive: %w", err)
	}
	defer file.Close()

	gr, err := gzip.NewReader(file)
	if err != nil {
		return "", fmt.Errorf("failed to initialize gzip reader: %w", err)
	}
	defer gr.Close()

	tr := tar.NewReader(gr)

	header, err := tr.Next()
	if err != nil {
		if errors.Is(err, io.EOF) {
			return "", fmt.Errorf("archive is empty")
		}
		return "", fmt.Errorf("failed to read tar entry: %w", err)
	}

	headerID, exists := header.PAXRecords["headerID"]
	if !exists {
		return "", fmt.Errorf("headerID not found in this archive's metadata")
	}

	return headerID, nil
}