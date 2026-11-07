<?php


use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    protected $connection = 'repository';

    public function up(): void
    {
        Schema::connection('repository')->create('packages', function (Blueprint $table) {
            $table->id();

            $table->string('name');
            $table->string('version');
            $table->text('description')->nullable();

            $table->string('filename')->unique();
            $table->unsignedBigInteger('size');

            $table->timestamp('uploaded_at')->useCurrent();

            $table->unique(['name', 'version']);
        });
    }

    public function down(): void
    {
        Schema::connection('repository')->dropIfExists('packages');
    }
};