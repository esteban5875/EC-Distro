<?php

namespace App\Services\Repository;

use Illuminate\Support\Facades\DB;
use Illuminate\Support\Facades\Artisan;
use RuntimeException;

class RepositoryIndex
{
    private string $db;

    public function __construct()
    {
        $this->db = dirname(base_path()) . '/repo/repository.sqlite';
    }

    
    public function initialize(): void
    {
        $directory = dirname($this->db);

        if (!is_dir($directory)) {
            mkdir($directory, 0755, true);
        }

        if (!file_exists($this->db)) {
            touch($this->db);
        }

        config([
            'database.connections.repository.database' => $this->db
        ]);

        DB::purge('repository');
        DB::connection('repository')->getPdo();

        //Run a migration automatically
        $exitCode = Artisan::call('migrate', [
            '--database' => 'repository',
            '--path' => 'database/migrations/repository',
            '--force' => true,
        ]);

        if ($exitCode !== 0) {
            throw new RuntimeException(
                "Repository migration failed: " . Artisan::output()
            );
        }
    }
}
