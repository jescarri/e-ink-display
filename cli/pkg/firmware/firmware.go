package firmware

import (
	"fmt"
	"io"
	"net/http"
	"time"
)

// Download downloads firmware from the specified URL
func Download(url string) ([]byte, error) {
	// Create HTTP client with timeout
	client := &http.Client{
		Timeout: 60 * time.Second,
	}

	// Send GET request
	resp, err := client.Get(url)
	if err != nil {
		return nil, fmt.Errorf("failed to download firmware: %w", err)
	}
	defer resp.Body.Close()

	// Check status code
	if resp.StatusCode != http.StatusOK {
		return nil, fmt.Errorf("download failed with status: %d %s", resp.StatusCode, resp.Status)
	}

	// Read response body
	data, err := io.ReadAll(resp.Body)
	if err != nil {
		return nil, fmt.Errorf("failed to read response body: %w", err)
	}

	return data, nil
}
