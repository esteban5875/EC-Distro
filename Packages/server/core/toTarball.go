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

func CreateTarball(targetDir, executablePath string) error {
	execFile, err := os.Open(executablePath)
	if err != nil {
		return fmt.Errorf("failed to open executable: %w", err)
	}
	defer execFile.Close()

	info, err := execFile.Stat()
	if err != nil {
		return fmt.Errorf("failed to get file stats: %w", err)
	}

	tarballName := info.Name() + ".tar.gz"
	tarballPath := filepath.Join(targetDir, tarballName)

	if err := os.MkdirAll(targetDir, 0755); err != nil {
		return fmt.Errorf("failed to create target directory: %w", err)
	}

	tarballFile, err := os.OpenFile(tarballPath, os.O_CREATE|os.O_WRONLY|os.O_EXCL, 0666)
	if err != nil {
		if errors.Is(err, os.ErrExist) {
			return fmt.Errorf("target archive path already exists: %s", tarballPath)
		}
		return fmt.Errorf("failed to create tarball file: %w", err)
	}
	defer tarballFile.Close()

	gw := gzip.NewWriter(tarballFile)
	defer gw.Close()

	tw := tar.NewWriter(gw)
	defer tw.Close()

	header, err := tar.FileInfoHeader(info, info.Name())
	if err != nil {
		return fmt.Errorf("failed to create tar header: %w", err)
	}
	header.Name = info.Name()

	if err := tw.WriteHeader(header); err != nil {
		return fmt.Errorf("failed to write header: %w", err)
	}

	if _, err := io.Copy(tw, execFile); err != nil {
		return fmt.Errorf("failed to copy file contents: %w", err)
	}

	fmt.Printf("Successfully created: %s\n", tarballPath)
	return nil
}
