<?php

use App\Services\Repository\RepositoryIndex;
use Tests\TestCase;


class InitDBTest extends TestCase
{
    public function test_repository_database_initializes_successfully(): void
    {
        $repository = new RepositoryIndex();

        $repository->initialize();

        $this->assertTrue(
            Schema::connection('repository')->hasTable('packages')
        );
    }
}