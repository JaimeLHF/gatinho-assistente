-- AlterTable
ALTER TABLE `device_customizations` ADD COLUMN `bgColor` VARCHAR(191) NOT NULL DEFAULT '#000000',
    ADD COLUMN `bgType` VARCHAR(191) NOT NULL DEFAULT 'solid';
